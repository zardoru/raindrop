#pragma once

#include <atomic>
#include <variant>

class SongLoader;

namespace otoworm {
    class ChartGroup;
}

class SongList;

struct ListEntry
{
    std::variant<std::shared_ptr<otoworm::ChartGroup>, std::shared_ptr<SongList>> data;
    std::string entry_name;
    int selected_index;
	ListEntry();
};

enum ESortCriteria
{
	SORT_UNKNOWN,
	SORT_TITLE,
	SORT_AUTHOR,
	SORT_LENGTH,
	SORT_MAXLEVEL,
	SORT_MINLEVEL,
	SORT_COUNT
};

typedef std::function<void()> OnLoadNotifyFunc;

class SongList
{
    SongList* m_parent_;
    std::vector<ListEntry> m_children_;
	std::atomic<bool> is_in_use_;
	void sort_by_fn(std::function<bool(const ListEntry&, const ListEntry&)> fn);

public:
    explicit SongList(SongList* parent = nullptr);
    ~SongList();

	void clear();

	void set_in_use(bool inuse);
	bool is_in_use();

	void clear_empty();

    void add_named_directory(std::mutex &load_mutex, SongLoader &loader, const std::filesystem::path &dir, std::string name, const OnLoadNotifyFunc &on_song_loaded = nullptr);
    void add_directory(std::mutex &load_mutex, SongLoader &loader, const std::filesystem::path &dir, const OnLoadNotifyFunc &on_song_loaded = nullptr);
    void add_song(const std::shared_ptr<otoworm::ChartGroup> &chart_group);
	
	void add_entry(const ListEntry &entry);
	const std::vector<ListEntry>& get_entries();

    // if false, it's a song
	bool is_directory(unsigned int entry) const;
    std::shared_ptr<SongList> get_list_entry(unsigned int entry) const;
    std::shared_ptr<otoworm::ChartGroup> get_song_entry(unsigned int entry) const;

    std::string get_entry_title(unsigned int entry);
    unsigned int get_num_entries() const;

    bool has_parent_directory() const;
    SongList* get_parent_directory() const;
	void sort_by(ESortCriteria criteria);
};
