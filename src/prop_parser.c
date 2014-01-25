#include "prop_parser.h"
#include "stringbuf.h"

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define NAME_EQ(x, y) \
    (xmlStrcmp((x)->name, (const xmlChar *)(y)) == 0)

#define STR_EQ(x, y) \
    (strcmp(x, y) == 0)

static xmlNode *advance_to_dict(xmlNode *root)
{
    if (root && NAME_EQ(root, "plist")) {
        for (root = root->children;
             root != NULL;
             root = root->next) {
            if (NAME_EQ(root, "dict")) {
                return root->children;
            }
        }
    }
    return NULL;
}

static void copy_tag_value_to(xmlDoc          *doc,
                              xmlNode         *node,
                              DocSetStringBuf *buf)
{
    xmlChar * name =
        xmlNodeListGetString(doc, node->xmlChildrenNode, 1);

    const char * name_string = (const char *)name;

    docset_sb_assign(buf, name_string, (size_t)strlen(name_string));

    xmlFree(name);
}

static DocSetProp *find_prop(const char *name,
                             DocSetProp *begin,
                             DocSetProp *end)
{
    DocSetProp *prop;
    for (prop = begin; prop != end; ++prop) {
        if (STR_EQ(name, prop->name)) break;
    }
    return prop;
}

int docset_parse_properties(const char *path,
                            DocSetProp *begin,
                            DocSetProp *end)
{
    xmlDoc     *doc;
    xmlNode    *root;
    xmlNode    *dict;
    DocSetProp *prop = end;
    int      success = 0;
    DocSetStringBuf buf;

    doc = xmlParseFile(path);

    if (!doc)
        return 0;

    root = xmlDocGetRootElement(doc);

    if (root == NULL)
        goto exit;

    if (!docset_sb_init(&buf, 20))
        goto exit;

    for (dict = advance_to_dict(root);
         dict != NULL;
         dict = dict->next) {

        if (NAME_EQ(dict, "key")) {
            copy_tag_value_to(doc, dict, &buf);
            prop = find_prop(buf.data, begin, end);
            continue;
        }

        if (prop != end
            && NAME_EQ(dict, "string")
            && prop->type == DOCSET_PROP_STRING) {

            copy_tag_value_to(doc, dict, &buf);
            *prop->target.str_target = docset_sb_new_string(&buf);
            prop = end;
            continue;
        }

        if (prop != end
            && (NAME_EQ(dict, "true") || NAME_EQ(dict, "false"))
            && prop->type == DOCSET_PROP_BOOL) {

            *prop->target.bool_target = NAME_EQ(dict, "true");
        }
    }

    success = 1;
    docset_sb_destroy(&buf);

exit:
    xmlFreeDoc(doc);
    return success;
}
