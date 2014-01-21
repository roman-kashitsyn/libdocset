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
    DOCSET_TYPE_UNKNOWN,
    DOCSET_TYPE_ATTRIBUTE,
    DOCSET_TYPE_BINDING,
    DOCSET_TYPE_BUILTIN,
    DOCSET_TYPE_CALLBACK,
    DOCSET_TYPE_CATEGORY,
    DOCSET_TYPE_CLASS,
    DOCSET_TYPE_COMMAND,
    DOCSET_TYPE_COMPONENT,
    DOCSET_TYPE_CONSTANT,
    DOCSET_TYPE_CONSTRUCTOR,
    DOCSET_TYPE_DEFINE,
    DOCSET_TYPE_DELEGATE,
    DOCSET_TYPE_DIRECTIVE,
    DOCSET_TYPE_ELEMENT,
    DOCSET_TYPE_ENTRY,
    DOCSET_TYPE_ENUM,
    DOCSET_TYPE_ERROR,
    DOCSET_TYPE_EVENT,
    DOCSET_TYPE_EXCEPTION,
    DOCSET_TYPE_FIELD,
    DOCSET_TYPE_FILE,
    DOCSET_TYPE_FILTER,
    DOCSET_TYPE_FRAMEWORK,
    DOCSET_TYPE_FUNCTION,
    DOCSET_TYPE_GLOBAL,
    DOCSET_TYPE_GUIDE,
    DOCSET_TYPE_INSTANCE,
    DOCSET_TYPE_INSTRUCTION,
    DOCSET_TYPE_INTERFACE,
    DOCSET_TYPE_KEYWORD,
    DOCSET_TYPE_LIBRARY,
    DOCSET_TYPE_LITERAL,
    DOCSET_TYPE_MACRO,
    DOCSET_TYPE_METHOD,
    DOCSET_TYPE_MIXIN,
    DOCSET_TYPE_MODULE,
    DOCSET_TYPE_NAMESPACE,
    DOCSET_TYPE_NOTATION,
    DOCSET_TYPE_OBJECT,
    DOCSET_TYPE_OPERATOR,
    DOCSET_TYPE_OPTION,
    DOCSET_TYPE_PACKAGE,
    DOCSET_TYPE_PARAMETER,
    DOCSET_TYPE_PROCEDURE,
    DOCSET_TYPE_PROPERTY,
    DOCSET_TYPE_PROTOCOL,
    DOCSET_TYPE_RECORD,
    DOCSET_TYPE_RESOURCE,
    DOCSET_TYPE_SAMPLE,
    DOCSET_TYPE_SECTION,
    DOCSET_TYPE_SERVICE,
    DOCSET_TYPE_STRUCT,
    DOCSET_TYPE_STYLE,
    DOCSET_TYPE_SUBROUTINE,
    DOCSET_TYPE_TAG,
    DOCSET_TYPE_TRAIT,
    DOCSET_TYPE_TYPE,
    DOCSET_TYPE_UNION,
    DOCSET_TYPE_VALUE,
    DOCSET_TYPE_VARIABLE
} DocSetEntryType;

/**
 * Structure representing string chunk.
 */
typedef struct {
    const char * data;
    unsigned size;
} DocSetStringRef;

/**
 * Abstract data type representing docset.
 */
struct DocSet;
typedef struct DocSet DocSet;

/**
 * Abstract data type representing ongoing docset search.
 */
struct DocSetSearch;
typedef struct DocSetSearch DocSetSearch;

/**
 * Opens a docset for reading.
 * @return pointer to DocSet on success,
 *         NULL on error.
 */
DocSet * docset_open(const char *basedir);

/**
 * Closes docset and frees all the allocated docset resources.
 */
int docset_close(DocSet *docset);

/** @defgroup meta Metadata Access
 *  @{
 */
/**
 * Returns docset kind.
 */
DocSetKind docset_kind(const DocSet * docset);

/**
 * Returns docset bundle identifier.
 * It's a unique identifier of a bundle.
 */
const char * docset_bundle_identifier(const DocSet * docset);

/**
 * Returns docset name. This name is usually used to display
 * it to a human.
 */
const char * docset_name(const DocSet * docset);

/**
 * Returns docset platform family. Families allow us to join several
 * docsets into a logical group. For example, Python_2 and Python_3
 * bundles could have a common family - 'python'.
 */
const char * docset_platform_family(const DocSet * docset);

/**
 * Returns string representation of docset kind.
 */
const char * docset_kind_name(DocSetKind kind);

/** @} */

/** @defgroup search Search API
 * @{
 */

/**
 * Creates in-memory index for fuzzy search if it's not created yet.
 * The index will be created automatically on first fuzzy search
 * request, but a separate function is still useful.
 */
int docset_make_fuzzy_index(DocSet * docset);

/**
 * Starts substring search on docset.
 */
DocSetSearch * docset_search(DocSet * docset, const char *symbol);

/**
 * Starts fuzzy search on docset.
 */
DocSetSearch * docset_search_fuzzy(DocSet * docset, const char *input);

/**
 * Disposes docset search.
 */
int docset_dispose_search(DocSetSearch *search);

/**
 * Returns non-zero integer if search has items to process.
 */
int docset_search_has_more(DocSetSearch * search);

/**
 * Returns current entry name.
 */
const char * docset_search_entry_name(const DocSetSearch *search);

/**
 * Returns type of the current entry.
 */
DocSetEntryType docset_search_entry_type(const DocSetSearch *search);

/**
 * Returns type name of the current entry.
 */
const char * docset_search_entry_type_name(const DocSetSearch *search);

/**
 * Returns current entry path. The path could contain anchor symbol
 * (#).
 */
const char * docset_search_entry_path(const DocSetSearch *search);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
