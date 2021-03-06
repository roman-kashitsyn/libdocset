#include <docset.hpp>
#include <utility>

namespace docset
{

// Error

error::error(const char *text) throw()
    : std::runtime_error(text)
{}

// Docset

doc_set::doc_set(const char *dirname)
    : doc_set(std::string(dirname))
{}

doc_set::doc_set(std::string dirname)
    : basedir_(std::move(dirname))
{
    init(basedir_.c_str());
}

std::size_t doc_set::count() const
{
    return ::docset_count(docset_.get());
}

const char *doc_set::name() const
{
    return ::docset_name(docset_.get());
}

const char *doc_set::platform_family() const
{
    return ::docset_platform_family(docset_.get());
}

const char *doc_set::bundle_identifier() const
{
    return ::docset_bundle_identifier(docset_.get());
}

iterator doc_set::begin() const
{
    return iterator(::docset_list_entries(docset_.get()));
}

entry_range doc_set::find(const char *query) const
{
    return entry_range(docset_find(docset_.get(), query));
}

entry_range doc_set::find(const std::string& query) const
{
    return entry_range(docset_find(docset_.get(), query.c_str()));
}

entry_range doc_set::find_by_ids(const std::vector<entry::id_type> &ids) const
{
    return entry_range(docset_find_by_ids(docset_.get(), &ids[0], ids.size()));
}

void doc_set::init(const char *dirname)
{
    ::DocSet *ds;
    ::DocSetError err = ::docset_try_open(&ds, dirname);
    if (err != ::DOCSET_OK) {
        throw error(::docset_error_string(err));
    }
    docset_ = std::shared_ptr<::DocSet>(ds, ::docset_close);
}

// Entry

bool entry::operator==(const entry &rhs) const
{
    return name_ == rhs.name_
        && path_ == rhs.path_
        && canonical_type_ == rhs.canonical_type_;
}

void entry::assign_raw_entry(::DocSetEntry *e)
{
    id_ = ::docset_entry_id(e);
    name_.assign(::docset_entry_name(e));
    path_.assign(::docset_entry_path(e));
    canonical_type_ = ::docset_entry_type(e);
}

// Iterator

iterator::iterator(DocSetCursor *cursor)
    : cursor_(cursor, ::docset_cursor_dispose)
{
    ++(*this);
}

iterator::iterator(const std::shared_ptr<DocSetCursor> &cursor)
    : cursor_(cursor)
{
    ++(*this);
}

iterator &iterator::operator++()
{
    if (::docset_cursor_step(cursor_.get())) {
        ::DocSetEntry *e = docset_cursor_entry(cursor_.get());
        entry_.assign_raw_entry(e);
    } else {
        cursor_.reset();
    }
    return *this;
}

bool iterator::operator==(const iterator &rhs) const
{
    if (rhs.cursor_.get() == nullptr) {
        return cursor_.get() == nullptr;
    }
    return cursor_ == rhs.cursor_
        && entry_ == rhs.entry_;
}

// Entry range

entry_range::entry_range(::DocSetCursor *cursor)
    : cursor_(cursor, ::docset_cursor_dispose)
{}

iterator entry_range::begin() const
{
    return iterator(cursor_);
}

}
