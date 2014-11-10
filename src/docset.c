#include "docset.h"
#include "stringbuf.h"
#include "prop_parser.h"
#include "paths.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DASH_BASE_QUERY                                             \
    "select id, name, type, null as parent, path from searchIndex"

#define ZDASH_BASE_QUERY                                                \
    "select t.z_pk as id"                                               \
    ", t.ztokenname as name"                                            \
    ", tt.ztypename as type"                                            \
    ", null as parent"                                                  \
    ", coalesce(tf.zpath || '#' || tm.zanchor, tf.zpath) as path "      \
    "from ztoken t "                                                    \
    "join ztokentype tt on (t.ztokentype=tt.z_pk) "                     \
    "join ztokenmetainformation tm on (t.zmetainformation=tm.z_pk) "    \
    "join zfilepath tf on (tm.zfile=tf.z_pk)"

#define COLUMN_ORDERING " order by id"

#define BUF_INIT_SIZE 100

#define PLIST_FILE_NAME "Info.plist"
#define DB_FILE_NAME "docSet.dsidx"

#define DOCSET_SET_FLAG(flags, flag) ((flags) = (DocSetFlags)((flags) | (flag)))

static const char INDEX_FILE_PATH[] = "/Contents/Resources/" DB_FILE_NAME;
static const char INFO_PLIST_PATH[] = "/Contents/" PLIST_FILE_NAME;

static const char *KIND_NAMES[] = { "DASH", "ZDASH" };

static const char TABLE_COUNT_QUERY[] = "select count(*) from sqlite_master "
                                        "where type='table' and name=?";

typedef struct QueryTable
{
    const char *all_query;
    const char *name_like_query;
    const char *count_query;
    const char *query_base;
} QueryTable;

static QueryTable dash_query_table =
{
    DASH_BASE_QUERY COLUMN_ORDERING,
    DASH_BASE_QUERY " where name like ? " COLUMN_ORDERING,
    "select count(*) from searchIndex",
    DASH_BASE_QUERY
};

static QueryTable zdash_query_table =
{
    ZDASH_BASE_QUERY COLUMN_ORDERING,
    ZDASH_BASE_QUERY " where name like ? " COLUMN_ORDERING,
    "select count(*) from ztoken",
    ZDASH_BASE_QUERY
};

struct DocSet
{
    sqlite3 *db;
    QueryTable *query_table;
    DocSetFlags flags;

    char *bundle_id;
    char *name;
    char *platform_family;

    docset_err_handler err_handler;
    void *err_context;
};

struct DocSetEntry
{
    DocSetEntryId id;
    DocSetStringBuf name;
    DocSetStringBuf type;
    DocSetStringBuf parent;
    DocSetStringBuf path;
};

struct DocSetCursor
{
    DocSet *docset;
    DocSetEntry entry;
    sqlite3_stmt *stmt;
};

static int init_entry(DocSetEntry *e);

static int file_exists(const char *);

static void assign_buffer_col(DocSetStringBuf *buf,
                              sqlite3_stmt *stmt,
                              int col);

static int parse_props(DocSet *docset, const char *path);

static int set_query_table(DocSet *);

static void report_error(DocSet *docset, const char *message);

static void report_no_mem(DocSet *docset);

static int count_tables(sqlite3 *db, const char *table);

static void dispose_entry(DocSetEntry *entry);

static DocSetCursor *cursor_for_query(DocSet *docset,
                                      const char *query,
                                      int len);

DocSet *docset_open(const char *basedir)
{
    DocSet *ds = NULL;
    (void)docset_try_open(&ds, basedir);
    return ds;
}

DocSetError docset_try_open(DocSet **docset, const char *basedir)
{
    char *index_path = NULL;
    char *plist_path = NULL;
    size_t base_len = strlen(basedir);
    DocSetError err = DOCSET_OK;

    if (!docset || !basedir || !base_len) {
        return DOCSET_BAD_CALL;
    }

    *docset = (DocSet *) calloc(1, sizeof(**docset));

    if (!*docset) {
        return DOCSET_NO_MEM;
    }

    plist_path = (char *) malloc(base_len + sizeof(INFO_PLIST_PATH) + 1);

    if (plist_path == NULL) {
        err = DOCSET_NO_MEM;
        goto fail;
    }

    sprintf(plist_path, "%s%s", basedir, INFO_PLIST_PATH);

    if (!file_exists(plist_path)) {
        err = DOCSET_NO_INFO_FILE;
        goto fail;
    }
    if (!parse_props(*docset, plist_path)) {
        err = DOCSET_BAD_XML;
        goto fail;
    }

    index_path = (char *) malloc(base_len + sizeof(INDEX_FILE_PATH) + 1);

    if (index_path == NULL) {
        err = DOCSET_NO_MEM;
        goto fail;
    }

    sprintf(index_path, "%s%s", basedir, INDEX_FILE_PATH);
    if (sqlite3_open(index_path, &(*docset)->db) != SQLITE_OK) {
        err = DOCSET_BAD_DB;
        goto fail;
    }

    if (!set_query_table(*docset)) {
        err = DOCSET_BAD_DB;
        goto fail;
    }

    free(plist_path);
    free(index_path);

    return err;

fail:
    (void)docset_close(*docset);
    free(index_path);
    free(plist_path);
    return err;
}

DocSetError docset_close(DocSet *docset)
{
    int ret_code;

    if (!docset) {
        return DOCSET_OK;
    }

    ret_code = sqlite3_close(docset->db);
    free(docset->bundle_id);
    free(docset->name);
    free(docset->platform_family);
    free(docset);
    return ret_code == SQLITE_OK ? DOCSET_OK : DOCSET_BAD_DB;
}

unsigned int docset_count(DocSet *docset)
{
    sqlite3_stmt *stmt = NULL;
    unsigned int result = 0;
    const char *query = NULL;
    int error = 0;

    if (!docset) {
        return result;
    }

    query = docset->query_table->count_query;

    if (sqlite3_prepare_v2(docset->db, query, -1, &stmt, NULL) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    } else {
        error = 1;
    }

    sqlite3_finalize(stmt);

    if (error) report_error(docset, "Query execution error");

    return result;
}

const char *docset_name(DocSet *docset)
{
    return docset->name;
}

const char *docset_bundle_identifier(DocSet *docset)
{
    return docset->bundle_id;
}

const char *docset_platform_family(DocSet *docset)
{
    return docset->platform_family;
}

DocSetFlags docset_flags(DocSet *docset)
{
    return docset->flags;
}

void docset_set_error_handler(DocSet *docset, docset_err_handler h, void *ctx)
{
    if (docset) {
        docset->err_handler = h;
        docset->err_context = ctx;
    }
}

const char *docset_error_string(DocSetError err)
{
    switch (err) {
    case DOCSET_OK: return NULL;
    case DOCSET_BAD_CALL: return "Docset API usage error";
    case DOCSET_NO_MEM: return "Memory allocation error";
    case DOCSET_NO_INFO_FILE: return "File not found: " PLIST_FILE_NAME;
    case DOCSET_BAD_XML: return PLIST_FILE_NAME ": Xml parse error";
    case DOCSET_NO_DB: return "File not found: " DB_FILE_NAME;
    case DOCSET_BAD_DB: return DB_FILE_NAME ": Database access error";
    case DOCSET_TOO_MANY_ARGS: return "Too many arguments";
    default: return "Unknown docset error";
    }
}

const char *docset_kind_name(DocSetKind k)
{
    if (DOCSET_KIND_DASH <= k && k <= DOCSET_KIND_ZDASH) {
        return KIND_NAMES[k];
    }
    return "UNKNOWN";
}

DocSetKind docset_kind(DocSet *docset)
{
    QueryTable *t;

    if (docset == NULL) {
        return DOCSET_KIND_UNKNOWN;
    }

    t = docset->query_table;

    if (t == &dash_query_table) {
        return DOCSET_KIND_DASH;
    }
    if (t == &zdash_query_table) {
        return DOCSET_KIND_ZDASH;
    }

    return DOCSET_KIND_UNKNOWN;
}

DocSetCursor *docset_find(DocSet *docset, const char *pattern)
{
    DocSetCursor *cursor;
    const char *query;

    if (!docset || !pattern) {
        return NULL;
    }

    query = docset->query_table->name_like_query;
    cursor = cursor_for_query(docset, query, -1);

    if (!cursor) {
        return NULL;
    }

    sqlite3_bind_text(cursor->stmt, 1, pattern, -1, SQLITE_TRANSIENT);

    return cursor;
}

DocSetCursor *docset_find_by_ids(DocSet *docset,
                                 const DocSetEntryId *ids,
                                 unsigned num_ids)
{
    DocSetCursor *cursor = NULL;
    DocSetStringBuf buf;
    const char *query_base;
    size_t n;
    unsigned i;

    if (!docset || !ids || num_ids < 1) {
        report_error(docset, docset_error_string(DOCSET_BAD_CALL));
        return NULL;
    }
    if (num_ids > (unsigned) DOCSET_MAX_IDS) {
        report_error(docset, docset_error_string(DOCSET_TOO_MANY_ARGS));
        return NULL;
    }

    query_base = docset->query_table->query_base;
    n = strlen(query_base);

    docset_sb_init(&buf, n + num_ids * 5);
    docset_sb_assign(&buf, query_base, n);

    docset_sb_append(&buf, " where id in (?");
    for (i = 1; i < num_ids; i++) {
        docset_sb_append(&buf, ", ?");
    }
    docset_sb_append(&buf, ") order by id");

    cursor = cursor_for_query(docset, buf.data, -1);
    if (!cursor) {
        goto error;
    }

    for (i = 1; i <= num_ids; i++) {
        sqlite3_bind_int(cursor->stmt, (int)i, ids[i - 1]);
    }

    return cursor;

error:
    docset_sb_destroy(&buf);
    docset_cursor_dispose(cursor);
    return NULL;
}

DocSetCursor *docset_list_entries(DocSet *docset)
{
    const char *query;

    if (!docset) {
        return NULL;
    }

    query = docset->query_table->all_query;
    return cursor_for_query(docset, query, -1);
}

int docset_cursor_dispose(DocSetCursor *cursor)
{
    int ret_code;

    if (!cursor) {
        return 0;
    }

    ret_code = sqlite3_finalize(cursor->stmt);
    dispose_entry(&cursor->entry);
    free(cursor);

    return ret_code != SQLITE_OK;
}

int docset_cursor_step(DocSetCursor *cursor)
{
    return cursor && sqlite3_step(cursor->stmt) == SQLITE_ROW;
}

DocSetEntry *docset_cursor_entry(DocSetCursor *cursor)
{
    DocSetEntry *e;
    sqlite3_stmt *stmt;
    int column_index = 0;

    if (!cursor) {
        return NULL;
    }

    e = &cursor->entry;
    stmt = cursor->stmt;

    e->id = sqlite3_column_int(stmt, column_index++);
    assign_buffer_col(&e->name, stmt, column_index++);
    assign_buffer_col(&e->type, stmt, column_index++);
    assign_buffer_col(&e->parent, stmt, column_index++);
    assign_buffer_col(&e->path, stmt, column_index++);

    return e;
}

DocSetEntryId docset_entry_id(DocSetEntry *entry)
{
    return entry->id;
}

const char *docset_entry_name(DocSetEntry *entry)
{
    return entry->name.data;
}

DocSetEntryType docset_entry_type(DocSetEntry *entry)
{
    return docset_type_by_name(entry->type.data);
}

const char *docset_entry_type_name(DocSetEntry *entry)
{
    return entry->type.data;
}

const char *docset_entry_path(DocSetEntry *entry)
{
    return entry->path.data;
}

const char *docset_entry_canonical_type(DocSetEntry *entry)
{
    const char *native_type = docset_entry_type_name(entry);
    DocSetEntryType type = docset_type_by_name(native_type);

    return docset_canonical_type_name(type);
}

static int file_exists(const char *filename)
{
    /* Looks like the only ANSI-C compatible way
     * to check file existence */
    FILE *f = fopen(filename, "r");
    if (f != NULL) {
        fclose(f);
        return 1;
    }
    return 0;
}

static int init_entry(DocSetEntry *e)
{
    int ok = docset_sb_init(&e->name, BUF_INIT_SIZE) &
             docset_sb_init(&e->type, BUF_INIT_SIZE) &
             docset_sb_init(&e->parent, BUF_INIT_SIZE) &
             docset_sb_init(&e->path, BUF_INIT_SIZE);

    if (!ok) {
        dispose_entry(e);
    }

    return ok;
}

static void assign_buffer_col(DocSetStringBuf *buf, sqlite3_stmt *stmt, int col)
{
    docset_sb_assign(buf,
                     (const char *)sqlite3_column_text(stmt, col),
                     (size_t)sqlite3_column_bytes(stmt, col));
}

static int parse_props(DocSet *docset, const char *path)
{
    DocSetProp props[] = {
        { DOCSET_PROP_STRING, "CFBundleIdentifier", { 0 } },
        { DOCSET_PROP_STRING, "CFBundleName", { 0 } },
        { DOCSET_PROP_STRING, "DocSetPlatformFamily", { 0 } },
        { DOCSET_PROP_BOOL, "isDashDocset", { 0 } },
        { DOCSET_PROP_BOOL, "isJavaScriptEnabled", { 0 } },
    };
    const size_t num_props = sizeof(props) / sizeof(props[0]);
    int is_dash = 0;
    int js_enabled = 0;
    int result;

    props[0].target.str_target = &docset->bundle_id;
    props[1].target.str_target = &docset->name;
    props[2].target.str_target = &docset->platform_family;
    props[3].target.bool_target = &is_dash;
    props[4].target.bool_target = &js_enabled;

    result = docset_parse_properties(path, props, props + num_props);

    if (is_dash) {
        DOCSET_SET_FLAG(docset->flags, DOCSET_IS_DASH);
    }
    if (js_enabled) {
        DOCSET_SET_FLAG(docset->flags, DOCSET_IS_JS_ENABLED);
    }

    return result;
}

static int set_query_table(DocSet *docset)
{
    if (count_tables(docset->db, "searchIndex")) {
        docset->query_table = &dash_query_table;
        return 1;
    }

    if (count_tables(docset->db, "ZTOKEN")) {
        docset->query_table = &zdash_query_table;
        return 1;
    }

    return 0;
}

static DocSetCursor *cursor_for_query(DocSet *docset,
                                      const char *query,
                                      int len)
{
    sqlite3_stmt *stmt = NULL;
    DocSetCursor *c = (DocSetCursor *) calloc(1, sizeof(*c));

    if (!c) {
        report_no_mem(docset);
        return NULL;
    }

    if (!init_entry(&c->entry)) {
        free(c);
        report_no_mem(docset);
        return NULL;
    }

    if (sqlite3_prepare_v2(docset->db, query, len, &stmt, NULL) != SQLITE_OK) {
        docset_cursor_dispose(c);
        report_error(docset, "Can't prepare a query");
        return NULL;
    }

    c->docset = docset;
    c->stmt = stmt;
    return c;
}

static void report_error(DocSet *docset, const char *msg)
{
    if (docset && docset->err_handler) {
        docset->err_handler(docset->err_context, msg);
    }
}

static void report_no_mem(DocSet *docset)
{
    report_error(docset, docset_error_string(DOCSET_NO_MEM));
}

static int count_tables(sqlite3 *db, const char *table)
{
    sqlite3_stmt *stmt;
    int result = 0;

    if (sqlite3_prepare_v2(
            db, TABLE_COUNT_QUERY, sizeof(TABLE_COUNT_QUERY), &stmt, NULL) ==
        SQLITE_OK) {

        sqlite3_bind_text(stmt, 1, table, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    return result;
}

static void dispose_entry(DocSetEntry *e)
{
    docset_sb_destroy(&e->name);
    docset_sb_destroy(&e->type);
    docset_sb_destroy(&e->parent);
    docset_sb_destroy(&e->path);
}
