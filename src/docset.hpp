#ifndef LIBDOCSET_DOCSET_HPP
#define LIBDOCSET_DOCSET_HPP

/**
 * @file
 *
 * This file provides C++ bingings for the libdocset library.
 * Example:
 * @code
 *     docset::doc_set ds("C++.docset");
 *     // list entries matching query
 *     for (const auto &e: ds.find("std::%print%")) {
 *         std::cout << e.name() << ": " << e.path() << "\n";
 *     }
 *
 *     // list all entries
 *     for (const auto &e: ds) {
 *         std::cout << e.name() << ": " << e.path() << "\n";
 *     }
 *
 * @endcode
 */
#include <docset.h>
#include <string>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <memory>

namespace docset
{

class error : public std::runtime_error
{
public:
    error(const char *) throw();
};

class entry
{
public:
    typedef ::DocSetEntryId id_type;

    id_type id() const { return id_; }

    std::string name() const { return name_; }

    std::string path() const { return path_; }

    std::string type_name() const { return type_name_; }

    ::DocSetEntryType canonical_type() const { return canonical_type_; }

    const char *canonical_type_name() const
    {
        return ::docset_canonical_type_name(canonical_type_);
    }

    bool operator==(const entry &rhs) const;
    bool operator!=(const entry &rhs) const { return !(*this == rhs); }

private:
    friend class iterator;
    void assign_raw_entry(::DocSetEntry *);

private:
    id_type id_;
    std::string name_;
    std::string path_;
    std::string type_name_;
    ::DocSetEntryType canonical_type_;
};

/// @brief Iterator that traverses entries in a result set.
///
/// Models input iterator.
class iterator :
        public std::iterator<entry,
                             std::input_iterator_tag>
{
public:

    iterator(DocSetCursor *cursor = nullptr);
    iterator(const std::shared_ptr<DocSetCursor> &cursor);

    iterator &operator++();

    iterator operator++(int)
    {
        iterator copy(*this);
        ++copy;
        return copy;
    }

    const entry &operator*() const { return entry_; }

    const entry *operator->() const { return &entry_; }

    bool operator==(const iterator &rhs) const;

    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

private:
    std::shared_ptr<::DocSetCursor> cursor_;
    entry entry_;
};

/// @brief Represents query result set.
///
/// It's not safe to traverse the result set multiple times.
class entry_range
{
public:
    entry_range(::DocSetCursor *cursor);

    /// @brief Returns iterator pointing to the first entry
    /// in a result set.
    iterator begin() const;

    /// @brief Returns iterator pointing after the last valid entry
    /// in a result set.
    iterator end() const { return iterator(); }

private:
    std::shared_ptr<::DocSetCursor> cursor_;
};

/// @brief Represents a docset handle.
class doc_set
{
public:
    doc_set(const char *dirname);
    doc_set(std::string dirname);

    /// @brief Returns number of entries in a docset.
    std::size_t count() const;

    /// @brief Returns the name of a docset.
    const char *name() const;

    /// @brief Returns the platform family of a docset.
    const char *platform_family() const;

    /// @brief Returns the bundle identifier of a docset.
    const char *bundle_identifier() const;

    /// @brief Returns base docset directory.
    std::string basedir() const { return basedir_; }


    /// @brief Returns iterator pointing to the first entry in a docset.
    iterator begin() const;

    /// @brief Returns iterator pointing after the last entry in a docset.
    iterator end() const { return iterator(); }

    /// @brief Returns range of entries matching the given query.
    entry_range find(const char *query) const;

    entry_range find(const std::string &query) const;

    entry_range find_by_ids(const std::vector<entry::id_type> &ids) const;

private:
    void init(const char *);

private:
    std::string basedir_;
    std::shared_ptr<::DocSet> docset_;
};

}

#endif
