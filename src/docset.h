/**
 * @file
 *
 * This provides a documentation sets (used by the Dash application)
 * reading and searching library.
 *
 * Some details regarding the docset structure could be found at
 * http://kapeli.com/docsets
 *
 * Example:
 * @code
 *     DocSet *docset = docset_open("C.docset");
 *     DocSetCursor * c = docset_find(docset, "printf");
 *     DocSetEntry * e;
 *     while (docset_cursor_step(c)) {
 *         e = docset_cursor_entry(c);
 *         printf("%s: %s\n",
 *                docset_entry_name(e),
 *                docset_entry_path(e));
 *     }
 * @endcode
 */
#ifndef DOCSET_H
#define DOCSET_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief An enum for all known docset kinds.
 */
typedef enum {
    DOCSET_KIND_DASH,
    DOCSET_KIND_ZDASH
} DocSetKind;

/**
 * @brief An enum for all known entry types.
 */
typedef enum {
    DOCSET_TYPE_UNKNOWN = -1,
    DOCSET_TYPE_ATTRIBUTE = 0,
    DOCSET_TYPE_BINDING = 1,
    DOCSET_TYPE_BUILTIN = 2,
    DOCSET_TYPE_CALLBACK = 3,
    DOCSET_TYPE_CATEGORY = 4,
    DOCSET_TYPE_CLASS = 5,
    DOCSET_TYPE_COMMAND = 6,
    DOCSET_TYPE_COMPONENT = 7,
    DOCSET_TYPE_CONSTANT = 8,
    DOCSET_TYPE_CONSTRUCTOR = 9,
    DOCSET_TYPE_DEFINE = 10,
    DOCSET_TYPE_DELEGATE = 11,
    DOCSET_TYPE_DIRECTIVE = 12,
    DOCSET_TYPE_ELEMENT = 13,
    DOCSET_TYPE_ENTRY = 14,
    DOCSET_TYPE_ENUM = 15,
    DOCSET_TYPE_ERROR = 16,
    DOCSET_TYPE_EVENT = 17,
    DOCSET_TYPE_EXCEPTION = 18,
    DOCSET_TYPE_FIELD = 19,
    DOCSET_TYPE_FILE = 20,
    DOCSET_TYPE_FILTER = 21,
    DOCSET_TYPE_FRAMEWORK = 22,
    DOCSET_TYPE_FUNCTION = 23,
    DOCSET_TYPE_GLOBAL = 24,
    DOCSET_TYPE_GUIDE = 25,
    DOCSET_TYPE_INSTANCE = 26,
    DOCSET_TYPE_INSTRUCTION = 27,
    DOCSET_TYPE_INTERFACE = 28,
    DOCSET_TYPE_KEYWORD = 29,
    DOCSET_TYPE_LIBRARY = 30,
    DOCSET_TYPE_LITERAL = 31,
    DOCSET_TYPE_MACRO = 32,
    DOCSET_TYPE_METHOD = 33,
    DOCSET_TYPE_MIXIN = 34,
    DOCSET_TYPE_MODULE = 35,
    DOCSET_TYPE_NAMESPACE = 36,
    DOCSET_TYPE_NOTATION = 37,
    DOCSET_TYPE_OBJECT = 38,
    DOCSET_TYPE_OPERATOR = 39,
    DOCSET_TYPE_OPTION = 40,
    DOCSET_TYPE_PACKAGE = 41,
    DOCSET_TYPE_PARAMETER = 42,
    DOCSET_TYPE_PROCEDURE = 43,
    DOCSET_TYPE_PROPERTY = 44,
    DOCSET_TYPE_PROTOCOL = 45,
    DOCSET_TYPE_RECORD = 46,
    DOCSET_TYPE_RESOURCE = 47,
    DOCSET_TYPE_SAMPLE = 48,
    DOCSET_TYPE_SECTION = 49,
    DOCSET_TYPE_SERVICE = 50,
    DOCSET_TYPE_STRUCT = 51,
    DOCSET_TYPE_STYLE = 52,
    DOCSET_TYPE_SUBROUTINE = 53,
    DOCSET_TYPE_TAG = 54,
    DOCSET_TYPE_TRAIT = 55,
    DOCSET_TYPE_TYPE = 56,
    DOCSET_TYPE_UNION = 57,
    DOCSET_TYPE_VALUE = 58,
    DOCSET_TYPE_VARIABLE = 59,
    DOCSET_LAST_TYPE = DOCSET_TYPE_VARIABLE
} DocSetEntryType;

/**
 * @brief Abstract data type representing docset.
 */
typedef struct DocSet DocSet;

/**
 * @brief Abstract data type describing docset entry.
 */
typedef struct DocSetEntry DocSetEntry;

/**
 * @brief Abstract data type representing ongoing docset search.
 */
typedef struct DocSetCursor DocSetCursor;

typedef void (*docset_err_handler)(void *, const char *);

/**
 * @brief Opens a docset for reading.
 *
 * @return pointer to DocSet on success,
 *         NULL on error.
 */
DocSet * docset_open(const char *basedir);

/**
 * @brief Closes docset and frees all the allocated docset resources.
 */
int docset_close(DocSet *docset);

/**
 * @brief Sets function that will be called on error with specified
 * context.
 */
void docset_set_error_handler(DocSet *docset,
                              docset_err_handler h,
                              void * ctx);

/** @defgroup meta Metadata Access
 *  @{
 */
/**
 * @brief Returns docset kind.
 */
DocSetKind docset_kind(const DocSet * docset);

/**
 * @brief Returns docset bundle identifier. It's usually useful for
 * computers.
 */
const char * docset_bundle_identifier(const DocSet * docset);

/**
 * @brief Returns docset name. This name is usually used to display it
 * to a human.
 */
const char * docset_name(const DocSet * docset);

/**
 * @brief Returns docset platform family.
 *
 * Families allow us to join several docsets into a logical group. For
 * example, Python_2 and Python_3 bundles could have a common family -
 * 'python'.
 */
const char * docset_platform_family(const DocSet * docset);

/**
 * @brief Returns string representation of docset kind.
 */
const char * docset_kind_name(DocSetKind kind);

/** @} */


/** @defgroup search Search API
 * @{
 */

/**
 * @brief Starts substring search on docset.
 */
DocSetCursor * docset_find(DocSet * docset, const char * input);

/**
 * @brief Starts fuzzy search on a docset.
 */
DocSetCursor * docset_fuzzy_find(DocSet * docset, const char * input);

/**
 * @brief Disposes a cursor.
 */
int docset_cursor_dispose(DocSetCursor * cursor);

/**
 * @brief Advances the cursor to the next entry.
 * @return non-zero if cursor points to a valid item
 */
int docset_cursor_step(DocSetCursor * cursor);

/**
 * @brief Returns entry this cursor points to.
 *
 * @note The entry is owned by a cursor. Client MUST NOT dispose this
 * entry.
 */
const DocSetEntry * docset_cursor_entry(DocSetCursor * cursor);

/** @} */

/** @defgroup entry Entry manipulation functions
 *  @{ */

/**
 * @brief Returns current entry name.
 */
const char * docset_entry_name(const DocSetEntry * entry);

/**
 * @brief Returns type of the current entry.
 */
DocSetEntryType docset_entry_type(const DocSetEntry * entry);

/**
 * @brief Returns current entry path. The path could contain the
 * anchor symbol (#).
 */
const char * docset_entry_path(const DocSetEntry * entry);

/**
 * @brief Returns type name of the entry as it's recorded in the
 * index.
 */
const char * docset_entry_type_name(const DocSetEntry * entry);

/**
 * @brief Returns canonical name type of entry.
 *
 * @see docset_canonical_type_name
 */
const char * docset_entry_canonical_type(const DocSetEntry * entry);

/** @} */


/** @defgroup docset_types Docset Entry Types Manipulation
 *  @{  */

/**
 * @brief Maps symbolic type name into appropriate numerical constant.
 *
 * @return known entry type on successful matching or
 *         DOCSET_TYPE_UNKNOWN if no matching found.
 */
DocSetEntryType docset_type_by_name(const char *name);

/**
 * @brief Returns canonical name of entry type.
 */
const char * docset_canonical_type_name(DocSetEntryType type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
