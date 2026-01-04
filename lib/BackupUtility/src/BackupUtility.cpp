// file BackupUtility.cpp:

#include "BackupUtility/BackupUtility.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <sqlite3.h>
#include <xxhash.h>

namespace fs = std::filesystem;

inline const char* ChangeTypeToString(ChangeType type)
{
    switch (type)
    {
    case ChangeType::Unchanged:
        return "Unchanged";
    case ChangeType::Added:
        return "Added";
    case ChangeType::Modified:
        return "Modified";
    case ChangeType::Deleted:
        return "Deleted";
    }
    return "Unknown";
}

inline ChangeType StringToChangeType(const std::string& s)
{
    if (s == "Unchanged")
        return ChangeType::Unchanged;
    if (s == "Added")
        return ChangeType::Added;
    if (s == "Modified")
        return ChangeType::Modified;
    if (s == "Deleted")
        return ChangeType::Deleted;
    return ChangeType::Unchanged;
}

/* =========================
   Database helpers
   ========================= */

static bool OpenDatabase(const fs::path& path, sqlite3** db) { return sqlite3_open(path.string().c_str(), db) == SQLITE_OK; }

static void CloseDatabase(sqlite3* db)
{
    if (db)
        sqlite3_close(db);
}

static bool InitSchema(sqlite3* db)
{
    static const char* sql = "CREATE TABLE IF NOT EXISTS files ("
                             "path TEXT PRIMARY KEY,"
                             "hash TEXT NOT NULL,"
                             "last_updated TEXT NOT NULL,"
                             "status TEXT NOT NULL);"; // <-- new column

    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

static bool UpdateStoredFileState(sqlite3* db, const std::string& path, const std::string& hash, ChangeType status, const std::string& timestamp)
{
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db,
                           "INSERT INTO files(path, hash, status, last_updated) "
                           "VALUES(?1, ?2, ?3, ?4) "
                           "ON CONFLICT(path) DO UPDATE SET "
                           "hash=excluded.hash, status=excluded.status, last_updated=excluded.last_updated;",
                           -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, ChangeTypeToString(status), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

static bool GetStoredFileState(sqlite3* db, const std::string& path, std::string& outHash, ChangeType& outStatus, std::string& outTimestamp)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT hash, status, last_updated FROM files WHERE path=?1;", -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char* txtHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* txtStatus = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* txtTimestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if (txtHash && txtStatus && txtTimestamp)
        {
            outHash = txtHash;
            outStatus = StringToChangeType(txtStatus);
            outTimestamp = txtTimestamp;
            found = true;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

/* =========================
   Hashing
   ========================= */

static bool HashFile(const fs::path& file, std::string& outHash)
{
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return false;

    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0);

    char buf[8192];
    while (in.read(buf, sizeof(buf)))
        XXH64_update(state, buf, sizeof(buf));

    if (in.gcount() > 0)
        XXH64_update(state, buf, in.gcount());

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);

    std::ostringstream out;
    out << std::hex << hash;
    outHash = out.str();
    return true;
}

/* =========================
   Snapshot helpers
   ========================= */

inline std::string GetCurrentTimestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm tm{};

#ifdef _WIN32
    _localtime64_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm);
    return buf;
}

static fs::path CreateSnapshotDir(const fs::path& historyRoot)
{
    fs::path dir = historyRoot / GetCurrentTimestamp();
    fs::create_directories(dir);
    return dir;
}

static fs::path EnsureSnapshot(fs::path& snapshot, bool& snapshotCreated, const fs::path& historyRoot)
{
    if (!snapshotCreated)
    {
        snapshot = CreateSnapshotDir(historyRoot);
        snapshotCreated = true;
    }
    return snapshot;
}

/* =========================
   File processing
   ========================= */

static void ProcessFile(const fs::path& abs, const fs::path& sourceRoot, const fs::path& currentDir, sqlite3* db, fs::path& snapshot, bool& snapshotCreated,
                        const fs::path& historyDir, // pass central historyDir
                        std::function<void(const BackupProgress&)> onProgress, std::size_t& processed)
{
    std::error_code ec;
    fs::path rel = fs::relative(abs, sourceRoot, ec);
    fs::path currentFile = currentDir / rel;

    std::string newHash;
    if (!HashFile(abs, newHash))
        return;

    std::string oldHash;
    ChangeType oldStatus;
    std::string oldTimestamp;
    bool hadOld = db && GetStoredFileState(db, rel.string(), oldHash, oldStatus, oldTimestamp) && ChangeType::Deleted != oldStatus;
    bool changed = !hadOld || newHash != oldHash;

    if (changed && hadOld && fs::exists(currentFile, ec))
    {
        EnsureSnapshot(snapshot, snapshotCreated, historyDir);
        fs::path archived = snapshot / rel;
        fs::create_directories(archived.parent_path(), ec);
        fs::copy_file(currentFile, archived, fs::copy_options::overwrite_existing, ec);
    }

    ChangeType newStatus;
    if (!hadOld)
    {
        newStatus = ChangeType::Added;
        fs::create_directories(currentFile.parent_path(), ec);
        fs::copy_file(abs, currentFile, fs::copy_options::overwrite_existing, ec);
    }
    else if (changed)
    {
        newStatus = ChangeType::Modified;
        EnsureSnapshot(snapshot, snapshotCreated, currentDir.parent_path() / "history");
        fs::path archived = snapshot / rel;
        fs::create_directories(archived.parent_path(), ec);
        fs::copy_file(currentFile, archived, fs::copy_options::overwrite_existing, ec);
        fs::copy_file(abs, currentFile, fs::copy_options::overwrite_existing, ec);
    }
    else
        newStatus = ChangeType::Unchanged;

    if (db)
    {
        std::string ts;
        if (newStatus != ChangeType::Unchanged)
            ts = GetCurrentTimestamp(); // Only update timestamp if status changed
        else
            ts = oldTimestamp;

        UpdateStoredFileState(db, rel.string(), newHash, newStatus, ts);
    }

    if (onProgress)
        onProgress({"collecting", ++processed, 0, abs});
}

/* =========================
   Deleted files detection helper
   ========================= */
inline void DetectDeletedFiles(sqlite3* db, const fs::path& sourceDir, const fs::path& currentDir,
                               const fs::path& historyDir, // central historyDir
                               fs::path& snapshot, bool& snapshotCreated, std::function<void(const BackupProgress&)> onProgress)
{
    std::error_code ec;
    if (!db)
        return;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT path, status FROM files;", -1, &stmt, nullptr) != SQLITE_OK)
        return;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char* dbPathCStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* statusCStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        if (!dbPathCStr || !statusCStr)
            continue;

        std::string dbPath = dbPathCStr;
        ChangeType status = StringToChangeType(statusCStr);

        // Skip files already marked as deleted
        if (status == ChangeType::Deleted)
            continue;

        fs::path sourceFile = sourceDir / dbPath;
        fs::path currentFile = currentDir / dbPath;

        if (!fs::exists(sourceFile, ec))
        {
            // Ensure snapshot exists
            EnsureSnapshot(snapshot, snapshotCreated, historyDir);

            // Archive deleted file
            if (fs::exists(currentFile, ec))
            {
                fs::path archived = snapshot / dbPath;
                fs::create_directories(archived.parent_path(), ec);
                fs::copy_file(currentFile, archived, fs::copy_options::overwrite_existing, ec);
            }

            // Remove current file
            fs::remove(currentFile, ec);

            // Update DB: mark as deleted and update timestamp
            sqlite3_stmt* delStmt = nullptr;
            if (sqlite3_prepare_v2(db, "UPDATE files SET status=?1, last_updated=?2 WHERE path=?3;", -1, &delStmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(delStmt, 1, ChangeTypeToString(ChangeType::Deleted), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(delStmt, 2, GetCurrentTimestamp().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(delStmt, 3, dbPath.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(delStmt);
            }
            sqlite3_finalize(delStmt);

            // Callback
            if (onProgress)
                onProgress({"deleted", 0, 0, dbPath});
        }
    }

    sqlite3_finalize(stmt);
}

/* =========================
   Main backup logic
   ========================= */

bool RunBackup(const BackupConfig& cfg)
{
    sqlite3* db = nullptr;
    std::error_code ec;

    const fs::path currentDir = cfg.backupRoot / "backup";
    const fs::path historyDir = cfg.backupRoot / "deleted";

    fs::create_directories(currentDir, ec);
    fs::create_directories(historyDir, ec);

    if (!OpenDatabase(cfg.databaseFile, &db))
        return false;

    if (!InitSchema(db))
    {
        CloseDatabase(db);
        return false;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    fs::path snapshot;
    bool snapshotCreated = false;
    std::size_t processed = 0;

    if (fs::is_regular_file(cfg.sourceDir, ec))
    {
        ProcessFile(cfg.sourceDir, cfg.sourceDir.parent_path(), currentDir, db, snapshot, snapshotCreated, historyDir, cfg.onProgress, processed);
    }
    else if (fs::is_directory(cfg.sourceDir, ec))
    {
        for (const auto& entry : fs::recursive_directory_iterator(cfg.sourceDir, ec))
        {
            if (!ec && entry.is_regular_file())
            {
                ProcessFile(entry.path(), cfg.sourceDir, currentDir, db, snapshot, snapshotCreated, historyDir, cfg.onProgress, processed);
            }
        }
    }
    else
    {
        return false;
    }

    DetectDeletedFiles(db, cfg.sourceDir, currentDir, historyDir, snapshot, snapshotCreated, cfg.onProgress);

    if (db)
    {
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
        CloseDatabase(db);
    }

    return true;
}
