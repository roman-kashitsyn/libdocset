#include "docset.h"

#include <string.h>
#include <assert.h>

#define Attribute "Attribute"
#define Binding "Binding"
#define Builtin "Builtin"
#define Callback "Callback"
#define Category "Category"
#define Class "Class"
#define Command "Command"
#define Component "Component"
#define Constant "Constant"
#define Constructor "Constructor"
#define Define "Define"
#define Delegate "Delegate"
#define Directive "Directive"
#define Element "Element"
#define Entry "Entry"
#define Enum "Enum"
#define Error "Error"
#define Event "Event"
#define Exception "Exception"
#define Field "Field"
#define File "File"
#define Filter "Filter"
#define Framework "Framework"
#define Function "Function"
#define Global "Global"
#define Guide "Guide"
#define Instance "Instance"
#define Instruction "Instruction"
#define Interface "Interface"
#define Keyword "Keyword"
#define Library "Library"
#define Literal "Literal"
#define Macro "Macro"
#define Method "Method"
#define Mixin "Mixin"
#define Module "Module"
#define Namespace "Namespace"
#define Notation "Notation"
#define Object "Object"
#define Operator "Operator"
#define Option "Option"
#define Package "Package"
#define Parameter "Parameter"
#define Procedure "Procedure"
#define Property "Property"
#define Protocol "Protocol"
#define Record "Record"
#define Resource "Resource"
#define Sample "Sample"
#define Section "Section"
#define Service "Service"
#define Struct "Struct"
#define Style "Style"
#define Subroutine "Subroutine"
#define Tag "Tag"
#define Trait "Trait"
#define Type "Type"
#define Union "Union"
#define Value "Value"
#define Variable "Variable"

typedef struct {
    const char * type_name;
    DocSetEntryType type;
} TypeNameMapping;

/*
 * NB(roman): Entries in this table MUST be sorted.
 */
static TypeNameMapping TYPE_NAME_TO_ID[] = {
    { Attribute, DOCSET_TYPE_ATTRIBUTE },
    { Binding, DOCSET_TYPE_BINDING },
    { Builtin, DOCSET_TYPE_BUILTIN },
    { Callback, DOCSET_TYPE_CALLBACK },
    { Category, DOCSET_TYPE_CATEGORY },
    { Class, DOCSET_TYPE_CLASS },
    { Command, DOCSET_TYPE_COMMAND },
    { Component, DOCSET_TYPE_COMPONENT },
    { Constant, DOCSET_TYPE_CONSTANT },
    { Constructor, DOCSET_TYPE_CONSTRUCTOR },
    { Define, DOCSET_TYPE_DEFINE },
    { Delegate, DOCSET_TYPE_DELEGATE },
    { Directive, DOCSET_TYPE_DIRECTIVE },
    { Element, DOCSET_TYPE_ELEMENT },
    { Entry, DOCSET_TYPE_ENTRY },
    { Enum, DOCSET_TYPE_ENUM },
    { Error, DOCSET_TYPE_ERROR },
    { Event, DOCSET_TYPE_EVENT },
    { Exception, DOCSET_TYPE_EXCEPTION },
    { Field, DOCSET_TYPE_FIELD },
    { File, DOCSET_TYPE_FILE },
    { Filter, DOCSET_TYPE_FILTER },
    { Framework, DOCSET_TYPE_FRAMEWORK },
    { Function, DOCSET_TYPE_FUNCTION },
    { Global, DOCSET_TYPE_GLOBAL },
    { Guide, DOCSET_TYPE_GUIDE },
    { Instance, DOCSET_TYPE_INSTANCE },
    { Instruction, DOCSET_TYPE_INSTRUCTION },
    { Interface, DOCSET_TYPE_INTERFACE },
    { Keyword, DOCSET_TYPE_KEYWORD },
    { Library, DOCSET_TYPE_LIBRARY },
    { Literal, DOCSET_TYPE_LITERAL },
    { Macro, DOCSET_TYPE_MACRO },
    { Method, DOCSET_TYPE_METHOD },
    { Mixin, DOCSET_TYPE_MIXIN },
    { Module, DOCSET_TYPE_MODULE },
    { Namespace, DOCSET_TYPE_NAMESPACE },
    { Notation, DOCSET_TYPE_NOTATION },
    { Object, DOCSET_TYPE_OBJECT },
    { Operator, DOCSET_TYPE_OPERATOR },
    { Option, DOCSET_TYPE_OPTION },
    { Package, DOCSET_TYPE_PACKAGE },
    { Parameter, DOCSET_TYPE_PARAMETER },
    { Procedure, DOCSET_TYPE_PROCEDURE },
    { Property, DOCSET_TYPE_PROPERTY },
    { Protocol, DOCSET_TYPE_PROTOCOL },
    { Record, DOCSET_TYPE_RECORD },
    { Resource, DOCSET_TYPE_RESOURCE },
    { Sample, DOCSET_TYPE_SAMPLE },
    { Section, DOCSET_TYPE_SECTION },
    { Service, DOCSET_TYPE_SERVICE },
    { Struct, DOCSET_TYPE_STRUCT },
    { Style, DOCSET_TYPE_STYLE },
    { Subroutine, DOCSET_TYPE_SUBROUTINE },
    { Tag, DOCSET_TYPE_TAG },
    { Trait, DOCSET_TYPE_TRAIT },
    { Type, DOCSET_TYPE_TYPE },
    { Union, DOCSET_TYPE_UNION },
    { Value, DOCSET_TYPE_VALUE },
    { Variable, DOCSET_TYPE_VARIABLE },

    /* Unspecified but seen in practice */

    { "cl", DOCSET_TYPE_CLASS },
    { "clconst", DOCSET_TYPE_CONSTANT },
    { "clm", DOCSET_TYPE_METHOD },
    { "func", DOCSET_TYPE_FUNCTION },
    { "macro", DOCSET_TYPE_MACRO },
    { "tdef", DOCSET_TYPE_TYPE },
};

static const char * TYPE_ID_TO_NAME[] = {
    Attribute,
    Binding,
    Builtin,
    Callback,
    Category,
    Class,
    Command,
    Component,
    Constant,
    Constructor,
    Define,
    Delegate,
    Directive,
    Element,
    Entry,
    Enum,
    Error,
    Event,
    Exception,
    Field,
    File,
    Filter,
    Framework,
    Function,
    Global,
    Guide,
    Instance,
    Instruction,
    Interface,
    Keyword,
    Library,
    Literal,
    Macro,
    Method,
    Mixin,
    Module,
    Namespace,
    Notation,
    Object,
    Operator,
    Option,
    Package,
    Parameter,
    Procedure,
    Property,
    Protocol,
    Record,
    Resource,
    Sample,
    Section,
    Service,
    Struct,
    Style,
    Subroutine,
    Tag,
    Trait,
    Type,
    Union,
    Value,
    Variable,
};

DocSetEntryType docset_type_by_name(const char *name)
{
    /* Actually, reimplement binary search here is much faster and
     * simpler than to use bsearch from the standard library. */
    int l = 0, h = sizeof(TYPE_NAME_TO_ID) / sizeof(TYPE_NAME_TO_ID[0]);
    while (l < h) {
        int m = (h + l) / 2; // We know that upper bound, no int overflow here.
        int i = strcmp(name, TYPE_NAME_TO_ID[m].type_name);
        if (i == 0) return TYPE_NAME_TO_ID[m].type;
        if (i < 0) h = m;
        else l = m + 1;
    }
    return DOCSET_TYPE_UNKNOWN;
}

const char * docset_canonical_type_name(DocSetEntryType type)
{
    if (0 <= type && type <= DOCSET_LAST_TYPE) {
        return TYPE_ID_TO_NAME[type];
    }
    return "Unknown";
}
