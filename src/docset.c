#include "docset.h"
#include "fuzzy_index.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char INDEX_FILE_PATH[] = "/Contents/Resources/docSet.dsidx";
static const char DOCS_DIR_PATH[]   = "/Contents/Resources/Documents";
static const char INFO_PLIST_PATH[] = "/Contents/Info.plist";

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

    DocSetFuzzyIndex * index;
};

typedef struct {
    int (*advance)(DocSetSearch *);
    int (*dispose)(DocSetSearch *);
} DocSetSearchVTable;

struct DocSetSearch {
    DocSetSearchVTable * vptr;
    DocSet *       docset;
    sqlite3_stmt * stmt;
    const char *   input;
    int            has_more;
};

static int set_kind(DocSet *);
static int count_tables(sqlite3 *, const char *);
static DocSetStringRef all_names_query(DocSetKind k);
static DocSetStringRef get_search_query(DocSetKind k);

/* vtable for simple search */
static int search_advance(DocSetSearch *);
static int search_dispose(DocSetSearch *);

static DocSetSearchVTable simple_search_vtbl = {
    search_advance,
    search_dispose
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
        goto close_db;
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

const char * docset_kind_name(DocSetKind k)
{
    switch (k) {
    case DOCSET_KIND_DASH:
        return "DASH";
    case DOCSET_KIND_ZDASH:
        return "ZDASH";
    default:
        return "UNKNOWN";
    }
}

DocSetKind docset_kind(const DocSet *docset)
{
    return docset->kind;
}

int docset_make_fuzzy_index(DocSet * docset)
{
    sqlite3_stmt * stmt;
    int rc;

    if (docset->index) return 0;
    /*
    docset->index = docset_fuzzy_index_create();

    if (!docset->index) return 1;

    rc = sqlite3_prepare_v2(docset->db, DASH_NAMES_QUERY,
                            sizeof(DASH_NAMES_QUERY),
                            &stmt, NULL);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int n = sqlite3_column_int(stmt, 1);
            const unsigned char * text = sqlite3_column_text(stmt, 2);
            int bytes = sqlite3_column_bytes(stmt, 2);

            if (bytes >= DOCSET_FUZZY_MIN_WORD) {
                docset_fuzzy_index_add_word(docset->index, n,
                                            (const char *) text, bytes);
            }
        }
    }
    sqlite3_finalize(stmt);
    */
    return rc;
}

DocSetSearch * docset_search(DocSet * docset, const char * input)
{
    DocSetSearch * s = calloc(1, sizeof(*s));
    DocSetStringRef q;
    sqlite3_stmt * stmt;

    if (!s) return NULL;

    q = get_search_query(docset->kind);

    if (sqlite3_prepare_v2(docset->db, q.data, q.size, &stmt, NULL)
        != SQLITE_OK) {
        search_dispose(s);
        return NULL;
    }
    sqlite3_bind_text(stmt, 1, input, -1, SQLITE_TRANSIENT);

    s->vptr = &simple_search_vtbl;
    s->docset = docset;
    s->input = input;
    s->stmt = stmt;
    s->has_more = sqlite3_step(stmt) == SQLITE_ROW;
    return s;
}

int docset_dispose_search(DocSetSearch * search)
{
    return search && search->vptr->dispose(search);
}

int docset_search_has_more(DocSetSearch * search)
{
    return search && search->has_more;
}

int docset_search_advance(DocSetSearch * search)
{
    return search && search->vptr->advance(search);
}

const char * docset_search_entry_name(DocSetSearch * search)
{
    return sqlite3_column_text(search->stmt, 0);
}

const char * docset_search_entry_type_name(DocSetSearch * search)
{
    return sqlite3_column_text(search->stmt, 1);
}

const char * docset_search_entry_canonical_type(DocSetSearch * search)
{
    const char * native_type = docset_search_entry_type_name(search);
    DocSetEntryType type = docset_type_by_name(native_type);
    return docset_canonical_type_name(type);
}

const char * docset_search_entry_path(DocSetSearch * search)
{
    return sqlite3_column_text(search->stmt, 3);
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

static int search_advance(DocSetSearch * s)
{
    int rc = sqlite3_step(s->stmt);
    s->has_more = (rc == SQLITE_ROW);
}

static int search_dispose(DocSetSearch * s)
{
    int rc = sqlite3_finalize(s->stmt);
    free(s);
    return rc;
}
