#include "docset.h"
#include "stringbuf.h"
#include "prop_parser.h"
#include "paths.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DASH_BASE_QUERY \
    "select name, type, null as parent, path from searchIndex"

#define ZDASH_BASE_QUERY     "select t.ztokenname as name "             \
    ", tt.ztypename as type "                                           \
    ", null as parent "                                                 \
    ", coalesce(tf.zpath || '#' || tm.zanchor, tf.zpath) as path "      \
    "from ztoken t "                                                    \
    "join ztokentype tt on (t.ztokentype=tt.z_pk) "                     \
    "join ztokenmetainformation tm on (t.zmetainformation=tm.z_pk) "    \
    "join zfilepath tf on (tm.zfile=tf.z_pk)"

#define COLUMN_ORDERING " order by name asc, type asc"

#define BUF_INIT_SIZE 100

static const char INDEX_FILE_PATH[] = "/Contents/Resources/docSet.dsidx";
static const char DOCS_DIR_PATH[]   = "/Contents/Resources/Documents/";
static const char INFO_PLIST_PATH[] = "/Contents/Info.plist";

static const char *KIND_NAMES[] = {"DASH", "ZDASH"};

static const char TABLE_COUNT_QUERY[] =
    "select count(*) from sqlite_master "
    "where type='table' and name=?";

static const char DASH_NAME_LIKE_QUERY[] =
    DASH_BASE_QUERY
    " where name like ?"
    COLUMN_ORDERING;

static const char DASH_FULL_QUERY[] =
    DASH_BASE_QUERY
    COLUMN_ORDERING;

static const char ZDASH_FULL_QUERY[] =
    ZDASH_BASE_QUERY
    COLUMN_ORDERING;

static const char ZDASH_NAME_LIKE_QUERY[] =
    ZDASH_BASE_QUERY
    " where name like ?"
    COLUMN_ORDERING;

struct DocSet {
    sqlite3    *db;
    DocSetKind  kind;
    DocSetFlags flags;

    char *bundle_id;
    char *name;
    char *platform_family;

    docset_err_handler err_handler;
    void              *err_context;
};

struct DocSetEntry
{
    DocSetStringBuf name;
    DocSetStringBuf type;
    DocSetStringBuf parent;
    DocSetStringBuf path;
};

struct DocSetCursor {
    DocSet       *docset;
    DocSetEntry   entry;
    sqlite3_stmt *stmt;
};

static int
init_entry(DocSetEntry *e);

static void
assign_buffer_col(DocSetStringBuf *buf,
                  sqlite3_stmt    *stmt,
                  int              col);

static int
parse_props(DocSet     *docset,
            const char *path);

static int
set_kind(DocSet *);

static void
report_error(DocSet     *docset,
             const char *message);

static void
report_no_mem(DocSet *docset);

static int
count_tables(sqlite3    *db,
             const char *table);

static void
dispose_entry(DocSetEntry *entry);

static DocSetCursor *
cursor_for_query(DocSet     *docset,
                 const char *query,
                 size_t      len);

static void
get_search_query(DocSetKind   k,
                 const char **query,
                 size_t      *size);

static void
get_list_query(DocSetKind   k,
               const char **query,
               size_t      *size);

DocSet *
docset_open(const char *basedir)
{
    size_t  base_len = strlen(basedir);
    char   *index_path;
    char   *plist_path;
    DocSet *docset = calloc(1, sizeof(*docset));

    if (!docset) return NULL;

    plist_path = malloc(base_len + sizeof(INFO_PLIST_PATH) + 1);

    if (plist_path == NULL)
        goto free_docset;

    sprintf(plist_path, "%s%s", basedir, INFO_PLIST_PATH);
    if (!parse_props(docset, plist_path))
        goto free_plist_path;

    index_path = malloc(base_len + sizeof(INDEX_FILE_PATH) + 1);

    if (index_path == NULL)
        goto free_plist_path;

    sprintf(index_path, "%s%s", basedir, INDEX_FILE_PATH);
    if (sqlite3_open(index_path, &docset->db) != SQLITE_OK)
        goto free_index_path;

    if (!set_kind(docset))
        goto close_db;

    free(plist_path);
    free(index_path);

    return docset;

close_db:
   sqlite3_close(docset->db);

free_index_path:
    free(index_path);

free_plist_path:
   free(plist_path);

free_docset:
    free(docset);

    return NULL;
}

int
docset_close(DocSet *docset)
{
    int ret_code;

    if (!docset) return 1;

    ret_code = sqlite3_close(docset->db);
    free(docset->bundle_id);
    free(docset->name);
    free(docset->platform_family);
    free(docset);
    return ret_code != SQLITE_OK;
}

const char *
docset_name(DocSet *docset)
{
    return docset->name;
}

const char *
docset_bundle_identifier(DocSet *docset)
{
    return docset->bundle_id;
}

const char *
docset_platform_family(DocSet *docset)
{
    return docset->platform_family;
}

DocSetFlags
docset_flags(DocSet *docset)
{
    return docset->flags;
}

void
docset_set_error_handler(DocSet            *docset,
                         docset_err_handler h,
                         void              *ctx)
{
    if (docset) {
        docset->err_handler = h;
        docset->err_context = ctx;
    }
}

const char *
docset_kind_name(DocSetKind k)
{
    if (DOCSET_KIND_DASH <= k && k <= DOCSET_KIND_ZDASH) {
        return KIND_NAMES[k];
    }
    return "UNKNOWN";
}

DocSetKind
docset_kind(DocSet *docset)
{
    return docset->kind;
}

DocSetCursor *
docset_find(DocSet     *docset,
            const char *input)
{
    DocSetCursor *cursor;
    const char   *query;
    size_t        len;

    if (!docset || !input) return NULL;

    get_search_query(docset->kind, &query, &len);
    cursor = cursor_for_query(docset, query, len);

    if (!cursor) return NULL;

    sqlite3_bind_text(cursor->stmt, 1,
                      input,
                      -1,
                      SQLITE_TRANSIENT);

    return cursor;
}

DocSetCursor *
docset_list_entries(DocSet *docset)
{
    const char   *query;
    size_t        len;

    get_list_query(docset->kind, &query, &len);
    return cursor_for_query(docset, query, len);
}

int
docset_cursor_dispose(DocSetCursor *cursor)
{
    int ret_code;

    if (!cursor) return 0;

    ret_code = sqlite3_finalize(cursor->stmt);
    dispose_entry(&cursor->entry);
    free(cursor);

    return ret_code != SQLITE_OK;
}

int docset_cursor_step(DocSetCursor *cursor)
{
    return cursor
        && sqlite3_step(cursor->stmt) == SQLITE_ROW;
}

DocSetEntry *
docset_cursor_entry(DocSetCursor *cursor)
{
    DocSetEntry  *e;
    sqlite3_stmt *stmt;

    if (!cursor) return NULL;

    e    = &cursor->entry;
    stmt = cursor->stmt;

    assign_buffer_col(&e->name, stmt, 0);
    assign_buffer_col(&e->type, stmt, 1);
    assign_buffer_col(&e->parent, stmt, 2);
    assign_buffer_col(&e->path, stmt, 3);

    return e;
}

const char *
docset_entry_name(DocSetEntry *entry)
{
    return entry->name.data;
}

DocSetEntryType
docset_entry_type(DocSetEntry *entry)
{
    return docset_type_by_name(entry->type.data);
}

const char *
docset_entry_type_name(DocSetEntry *entry)
{
    return entry->type.data;
}

const char *
docset_entry_path(DocSetEntry *entry)
{
    return entry->path.data;
}

const char *
docset_entry_canonical_type(DocSetEntry *entry)
{
    const char     *native_type = docset_entry_type_name(entry);
    DocSetEntryType type        = docset_type_by_name(native_type);

    return docset_canonical_type_name(type);
}

static int
init_entry(DocSetEntry *e)
{
    if (!docset_sb_init(&e->name,   BUF_INIT_SIZE))
        goto fail;

    if (!docset_sb_init(&e->type,   BUF_INIT_SIZE))
        goto destroy_name;

    if (!docset_sb_init(&e->parent, BUF_INIT_SIZE))
        goto destroy_type;

    if (!docset_sb_init(&e->path,   BUF_INIT_SIZE))
        goto destroy_parent;

    return 1;
destroy_parent:
    docset_sb_destroy(&e->parent);
destroy_type:
    docset_sb_destroy(&e->type);
destroy_name:
    docset_sb_destroy(&e->type);
fail:
    return 0;
}

static void
assign_buffer_col(DocSetStringBuf *buf,
                  sqlite3_stmt    *stmt,
                  int              col)
{
    docset_sb_assign(buf,
                     (const char *)sqlite3_column_text(stmt, col),
                     (size_t)sqlite3_column_bytes(stmt, col));
}

static int
parse_props(DocSet     *docset,
            const char *path)
{
    DocSetProp  props[] = {
        { DOCSET_PROP_STRING, "CFBundleIdentifier",   {0} },
        { DOCSET_PROP_STRING, "CFBundleName",         {0} },
        { DOCSET_PROP_STRING, "DocSetPlatformFamily", {0} },
        { DOCSET_PROP_BOOL,   "isDashDocset",         {0} },
        { DOCSET_PROP_BOOL,   "isJavaScriptEnabled",  {0} },
    };
    const size_t num_props = sizeof(props) / sizeof(props[0]);
    int is_dash    = 0;
    int js_enabled = 0;
    int result;

    props[0].target.str_target  = &docset->bundle_id;
    props[1].target.str_target  = &docset->name;
    props[2].target.str_target  = &docset->platform_family;
    props[3].target.bool_target = &is_dash;
    props[4].target.bool_target = &js_enabled;

    result = docset_parse_properties(path, props, props + num_props);

    if (is_dash)    docset->flags |= DOCSET_IS_DASH;
    if (js_enabled) docset->flags |= DOCSET_IS_JS_ENABLED;

    return result;
}

static int
set_kind(DocSet *docset)
{
    if (count_tables(docset->db, "searchIndex")) {
        docset->kind = DOCSET_KIND_DASH;
        return 1;
    }

    if (count_tables(docset->db, "ZTOKEN")) {
        docset->kind = DOCSET_KIND_ZDASH;
        return 1;
    }

    return 0;
}

static DocSetCursor *
cursor_for_query(DocSet     *docset,
                 const char *query,
                 size_t      len)
{
    DocSetCursor *c = calloc(1, sizeof(*c));
    sqlite3_stmt *stmt;

    if (!c) {
        report_no_mem(docset);
        goto fail;
    }

    if (!init_entry(&c->entry)) {
        report_no_mem(docset);
        goto free_cursor_mem;
    }

    if (sqlite3_prepare_v2(docset->db, query, len, &stmt, NULL)
        != SQLITE_OK) {
        report_error(docset, "Can't prepare a query");
        goto fin_statement;
    }

    c->docset = docset;
    c->stmt = stmt;
    return c;

fin_statement:
    sqlite3_finalize(stmt);
free_cursor_mem:
    free(c);
fail:
    return NULL;
}

static void
report_error(DocSet     *docset,
             const char *msg)
{
    if (docset->err_handler) {
        docset->err_handler(docset->err_context, msg);
    }
}

static void
report_no_mem(DocSet *docset)
{
    report_error(docset, "Memory allocation error");
}

static int
count_tables(sqlite3    *db,
             const char *table)
{
    sqlite3_stmt *stmt;
    int result = 0;

    if (sqlite3_prepare_v2(db, TABLE_COUNT_QUERY,
                           sizeof(TABLE_COUNT_QUERY),
                           &stmt, NULL) == SQLITE_OK) {

        sqlite3_bind_text(stmt, 1, table, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    return result;
}

static void
dispose_entry(DocSetEntry *e)
{
    docset_sb_destroy(&e->name);
    docset_sb_destroy(&e->type);
    docset_sb_destroy(&e->parent);
    docset_sb_destroy(&e->path);
}

static void
get_search_query(DocSetKind   kind,
                 const char **query,
                 size_t      *len)
{
    switch (kind) {
    case DOCSET_KIND_DASH:
        *query = DASH_NAME_LIKE_QUERY;
        *len   = sizeof(DASH_NAME_LIKE_QUERY);
        break;
    case DOCSET_KIND_ZDASH:
        *query = ZDASH_NAME_LIKE_QUERY;
        *len   = sizeof(ZDASH_NAME_LIKE_QUERY);
        break;
    }
}

static void
get_list_query(DocSetKind   kind,
               const char **query,
               size_t      *len)
{
    switch (kind) {
    case DOCSET_KIND_DASH:
        *query = DASH_FULL_QUERY;
        *len   = sizeof(DASH_FULL_QUERY);
        break;
    case DOCSET_KIND_ZDASH:
        *query = ZDASH_FULL_QUERY;
        *len   = sizeof(ZDASH_FULL_QUERY);
        break;
    }
}
