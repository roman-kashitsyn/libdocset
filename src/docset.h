/**
 * @file
 *
 * This provides a docsets reading and searching library.
 *
 * Some details regarding the docset structure could be found here
 * http://kapeli.com/docsets
 *
 * Example:
 * @code
 *     DocSet *docset = docset_open("C.docset");
 *     if (!docset) report_error();
 *     printf("%s", docset_name(docset));
 * @endcode
 */
#ifndef DOCSET_H
#define DOCSET_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An enum for all known docset kinds.
 */
typedef enum {
    DOCSET_KIND_DASH,
    DOCSET_KIND_ZDASH
} DocSetKind;

/**
 * An enum for all known entry types.
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
 * @brief Structure representing string chunk.
 */
typedef struct {
    const char * data;
    unsigned size;
} DocSetStringRef;

/**
 * @brief Abstract data type representing docset.
 */
struct DocSet;
typedef struct DocSet DocSet;

/**
 * @brief Abstract data type representing ongoing docset search.
 */
struct DocSetSearch;
typedef struct DocSetSearch DocSetSearch;

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
 * @brief Creates in-memory index for fuzzy search if it's not created
 * yet.
 *
 * The index will be created automatically on first fuzzy search
 * request, but a separate function is still useful.
 */
int docset_make_fuzzy_index(DocSet * docset);

/**
 * @brief Starts substring search on docset.
 */
DocSetSearch * docset_search(DocSet * docset, const char *symbol);

/**
 * @brief Starts fuzzy search on docset.
 */
DocSetSearch * docset_search_fuzzy(DocSet * docset, const char *input);

/**
 * @brief Disposes docset search.
 */
int docset_dispose_search(DocSetSearch *search);

/**
 * @brief Returns non-zero integer if search has items to process.
 */
int docset_search_has_more(DocSetSearch * search);

/**
 * @brief Returns current entry name.
 */
const char * docset_search_entry_name(DocSetSearch *search);

/**
 * @brief Returns type of the current entry.
 */
DocSetEntryType docset_search_entry_type(DocSetSearch *search);

/**
 * @brief Returns type name of the current entry.
 */
const char * docset_search_entry_type_name(DocSetSearch *search);

/**
 * @brief Returns canonical name type of current entry.
 *
 * @see docset_canonical_type_name
 */
const char * docset_search_entry_canonical_type(DocSetSearch * search);

/**
 * @brief Returns current entry path. The path could contain the
 * anchor symbol (#).
 */
const char * docset_search_entry_path(DocSetSearch *search);

/** @} */

/** @defgroup docset_types Docset Entry Types Manipulation
 *  @{
 */

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
