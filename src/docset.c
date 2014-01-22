#include "docset.h"
#include "stringbuf.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char INDEX_FILE_PATH[] = "/Contents/Resources/docSet.dsidx";
static const char DOCS_DIR_PATH[]   = "/Contents/Resources/Documents";
static const char INFO_PLIST_PATH[] = "/Contents/Info.plist";

static const char *KIND_NAMES[] = {"DASH", "ZDASH"};

static const char TABLE_COUNT_QUERY[] =
    "select count(*) from sqlite_master "
    "where type='table' and name=?";

static const char DASH_NAMES_QUERY[] =
    "select rowid, name from searchIndex";

static const char ZDASH_NAMES_QUERY[] =
    "select rowid, ztokenname from ztoken";

static const char DASH_NAME_LIKE_QUERY[] =
    "select name, type, null, path from searchIndex "
    "where name like ? "
    "order by name asc, type asc";

static const char ZDASH_NAME_LIKE_QUERY[] =
    "select t.ztokenname as name "
    ", tt.ztypename as type "
    ", null as parent "
    ", coalesce(tf.zpath || '#' || tm.zanchor, tf.zpath) as path "
    "from ztoken t "
    "join ztokentype tt on (t.ztokentype=tt.z_pk) "
    "join ztokenmetainformation tm on (t.zmetainformation=tm.z_pk) "
    "join zfilepath tf on (tm.zfile=tf.z_pk) "
    "where name like ? "
    "order by name asc, type asc";

struct DocSet {
    const char * basedir;
    sqlite3 *    db;
    DocSetKind   kind;

    const char * bundle_id;
    const char * name;
    const char * platform_family;

    docset_err_handler err_handler;
    void * err_context;
};

struct DocSetEntry
{
    DocSetStringBuf name;
    DocSetStringBuf type;
    DocSetStringBuf parent;
    DocSetStringBuf path;
};

typedef struct {
    int (*advance)(DocSetCursor *);
    int (*dispose)(DocSetCursor *);
} DocSetCursorVTable;

struct DocSetCursor {
    DocSetCursorVTable * vptr;
    DocSet * docset;
    DocSetEntry entry;
    sqlite3_stmt * stmt;
    const char * input;
};

static int init_entry(DocSetEntry * e);

static void assign_buffer_col(DocSetStringBuf *, sqlite3_stmt *, int);

static int set_kind(DocSet *);

static void report_error(DocSet *, const char *);

static void report_no_mem(DocSet *);

static int count_tables(sqlite3 *, const char *);

static void dispose_entry(DocSetEntry *);

/* static DocSetStringRef all_names_query(DocSetKind k); */

static DocSetStringRef get_search_query(DocSetKind k);

/* vtable for simple cursor */
static int cursor_advance(DocSetCursor *);
static int cursor_dispose(DocSetCursor *);

static DocSetCursorVTable simple_search_vtbl = {
    cursor_advance,
    cursor_dispose
};

DocSet * docset_open(const char *basedir)
{
    size_t path_len;
    char * index_path;
    DocSet * docset = calloc(1, sizeof(*docset));

    if (!docset) return NULL;

    path_len = strlen(basedir) + sizeof(INDEX_FILE_PATH);
    index_path = malloc(path_len);

    if (!index_path)
        goto free_docset;

    sprintf(index_path, "%s%s", basedir, INDEX_FILE_PATH);

    if (sqlite3_open(index_path, &docset->db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database at %s\n", index_path);
        goto free_index_path;
    }

    if (!set_kind(docset)) {
        fprintf(stderr, "Unable to set kind for db at %s\n", index_path);
        goto close_db;
    }

    free(index_path);

    return docset;

close_db:
   sqlite3_close(docset->db);

free_index_path:
    free(index_path);

free_docset:
    free(docset);

    return NULL;
}

int docset_close(DocSet *docset)
{
    int rc = sqlite3_close(docset->db);
    free(docset);
    return rc;
}

void docset_set_error_handler(DocSet * docset,
                              docset_err_handler h,
                              void * ctx)
{
    if (docset) {
        docset->err_handler = h;
        docset->err_context = ctx;
    }
}

const char * docset_kind_name(DocSetKind k)
{
    if (DOCSET_KIND_DASH <= k && k <= DOCSET_KIND_ZDASH) {
        return KIND_NAMES[k];
    }
    return "UNKNOWN";
}

DocSetKind docset_kind(const DocSet *docset)
{
    return docset->kind;
}

DocSetCursor * docset_find(DocSet * docset, const char * input)
{
    DocSetCursor * c = calloc(1, sizeof(*c));
    DocSetStringRef q;
    sqlite3_stmt * stmt;

    if (!c) {
        report_no_mem(docset);
        goto fail;
    }

    if (!init_entry(&c->entry)) {
        report_no_mem(docset);
        goto free_cursor_mem;
    }

    q = get_search_query(docset->kind);

    if (sqlite3_prepare_v2(docset->db, q.data, q.size, &stmt, NULL)
        != SQLITE_OK) {
        report_error(docset, "Query preparation error");
        goto fin_statement;
    }
    sqlite3_bind_text(stmt, 1, input, -1, SQLITE_TRANSIENT);

    c->vptr = &simple_search_vtbl;
    c->docset = docset;
    c->input = input;
    c->stmt = stmt;
    return c;

fin_statement:
    sqlite3_finalize(stmt);
free_cursor_mem:
    free(c);
fail:
    return NULL;
}

int docset_cursor_dispose(DocSetCursor * cursor)
{
    return cursor && cursor->vptr->dispose(cursor);
}

int docset_cursor_step(DocSetCursor * cursor)
{
    return cursor && cursor->vptr->advance(cursor);
}

const DocSetEntry * docset_cursor_entry(DocSetCursor * cursor)
{
    DocSetEntry * e;
    sqlite3_stmt * stmt;

    if (!cursor) return NULL;

    e = &cursor->entry;
    stmt = cursor->stmt;

    assign_buffer_col(&e->name, stmt, 0);
    assign_buffer_col(&e->type, stmt, 1);
    assign_buffer_col(&e->parent, stmt, 2);
    assign_buffer_col(&e->path, stmt, 3);

    return e;
}

const char * docset_entry_name(const DocSetEntry * entry)
{
    return entry->name.data;
}

const char * docset_entry_type_name(const DocSetEntry * entry)
{
    return entry->type.data;
}

const char * docset_entry_path(const DocSetEntry * entry)
{
    return entry->path.data;
}

const char * docset_entry_canonical_type(const DocSetEntry * entry)
{
    const char * native_type = docset_entry_type_name(entry);
    DocSetEntryType type = docset_type_by_name(native_type);
    return docset_canonical_type_name(type);
}

static int init_entry(DocSetEntry * e)
{
    if (!docset_sb_init(&e->name,   20)) goto fail;
    if (!docset_sb_init(&e->type,   20)) goto destroy_name;
    if (!docset_sb_init(&e->parent, 20)) goto destroy_type;
    if (!docset_sb_init(&e->path,   20)) goto destroy_parent;

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

static void assign_buffer_col(DocSetStringBuf * buf, sqlite3_stmt *stmt, int col)
{
    docset_sb_assign(buf,
                     (const char *)sqlite3_column_text(stmt, col),
                     (size_t)sqlite3_column_bytes(stmt, col));
}

static int set_kind(DocSet * docset)
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

static void report_error(DocSet * docset, const char *msg)
{
    if (docset->err_handler) {
        docset->err_handler(docset->err_context, msg);
    }
}

static void report_no_mem(DocSet * docset)
{
    report_error(docset, "Memory allocation error");
}

static int count_tables(sqlite3 *db, const char *table)
{
    sqlite3_stmt * stmt;
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

static void dispose_entry(DocSetEntry * e)
{
    docset_sb_destroy(&e->name);
    docset_sb_destroy(&e->type);
    docset_sb_destroy(&e->parent);
    docset_sb_destroy(&e->path);
}

/*
static DocSetStringRef all_names_query(DocSetKind k)
{
    DocSetStringRef q;
    switch (k) {
    case DOCSET_KIND_DASH:
        q.data = DASH_NAMES_QUERY;
        q.size = sizeof(DASH_NAMES_QUERY);
        break;
    case DOCSET_KIND_ZDASH:
        q.data = ZDASH_NAMES_QUERY;
        q.size = sizeof(ZDASH_NAMES_QUERY);
        break;
    }
    return q;
}
*/

static DocSetStringRef get_search_query(DocSetKind k)
{
    DocSetStringRef q;
    switch (k) {
    case DOCSET_KIND_DASH:
        q.data = DASH_NAME_LIKE_QUERY;
        q.size = sizeof(DASH_NAME_LIKE_QUERY);
        break;
    case DOCSET_KIND_ZDASH:
        q.data = ZDASH_NAME_LIKE_QUERY;
        q.size = sizeof(ZDASH_NAME_LIKE_QUERY);
        break;
    }
    return q;
}

static int cursor_advance(DocSetCursor * c)
{
    return sqlite3_step(c->stmt) == SQLITE_ROW;
}

static int cursor_dispose(DocSetCursor * c)
{
    int ret_code = sqlite3_finalize(c->stmt);
    dispose_entry(&c->entry);
    free(c);
    return ret_code;
}
