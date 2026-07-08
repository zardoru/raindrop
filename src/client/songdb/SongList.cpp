#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <ranges>

#include <ChartGroup.h>
#include <text_and_file_util.h>

#include "SongList.h"
#include "SongLoader.h"

ListEntry::ListEntry()
    : data(std::make_shared<SongList>()),
      selected_index(0)
{
}

SongList::SongList(SongList* parent)
    : m_parent_(parent),
      is_in_use_(false)
{
}

SongList::~SongList() = default;

void SongList::clear()
{
    m_children_.clear();
}

void SongList::set_in_use(bool inuse)
{
    is_in_use_ = inuse;
}

bool SongList::is_in_use()
{
    return is_in_use_;
}

void SongList::clear_empty()
{
    for (auto it = m_children_.begin(); it != m_children_.end();) {
        bool increase = true;

        if (std::holds_alternative<std::shared_ptr<SongList>>(it->data)) {
            auto list = std::get<std::shared_ptr<SongList>>(it->data);
            if (list->get_num_entries() == 0 && !list->is_in_use()) {
                it = m_children_.erase(it);
                increase = false;
            }
        }

        if (increase)
            ++it;
    }
}

void SongList::add_song(const std::shared_ptr<otoworm::ChartGroup>& chart_group)
{
    ListEntry new_entry;
    new_entry.data = chart_group;
    m_children_.push_back(new_entry);
}

void SongList::add_entry(const ListEntry& entry)
{
    m_children_.push_back(entry);
}

const std::vector<ListEntry>& SongList::get_entries()
{
    return m_children_;
}

void SongList::add_named_directory(
    std::mutex& load_mutex,
    SongLoader& loader,
    const std::filesystem::path& dir,
    std::string name,
    const OnLoadNotifyFunc& on_song_loaded)
{
    bool entry_was_pushed = false;
    auto new_list = std::make_shared<SongList>(this);

    ListEntry new_entry;
    new_entry.entry_name = std::move(name);
    new_entry.data = new_list;

    std::vector<std::shared_ptr<otoworm::ChartGroup>> chart_groups;

    if (!std::filesystem::exists(dir))
        return;

    for (const auto& i : std::filesystem::directory_iterator(dir)) {
        if (i.path() == "." || i.path() == "..")
            continue;

        if (!std::filesystem::is_directory(i.path()))
            continue;

        loader.LoadChartGroupsFromDir(i, chart_groups);

        if (chart_groups.empty()) {
            if (!entry_was_pushed) {
                std::unique_lock lock(load_mutex);
                m_children_.push_back(new_entry);
                entry_was_pushed = true;
            }

            new_list->add_directory(load_mutex, loader, i, on_song_loaded);

            {
                std::unique_lock lock(load_mutex);
                if (!new_list->get_num_entries() && !new_list->is_in_use()) {
                    if (!m_children_.empty())
                        m_children_.erase(m_children_.end() - 1);
                    entry_was_pushed = false;
                }
            }
        } else {
            {
                std::unique_lock lock(load_mutex);
                for (const auto& chart_group : chart_groups)
                    new_list->add_song(chart_group);

                chart_groups.clear();
            }

            if (!entry_was_pushed) {
                std::unique_lock lock(load_mutex);
                m_children_.push_back(new_entry);
                entry_was_pushed = true;
            }
        }

        if (entry_was_pushed && on_song_loaded)
            on_song_loaded();
    }
}

void SongList::add_directory(
    std::mutex& load_mutex,
    SongLoader& loader,
    const std::filesystem::path& dir,
    const OnLoadNotifyFunc& on_song_loaded)
{
    add_named_directory(load_mutex, loader, dir, otoworm::locale::wstring_to_utf8(dir.filename().wstring()), on_song_loaded);
}

bool SongList::is_directory(unsigned int entry) const
{
    if (entry >= m_children_.size())
        return true;

    return std::holds_alternative<std::shared_ptr<SongList>>(m_children_[entry].data);
}

std::shared_ptr<SongList> SongList::get_list_entry(unsigned int entry) const
{
    assert(is_directory(entry));
    return std::get<std::shared_ptr<SongList>>(m_children_[entry].data);
}

std::shared_ptr<otoworm::ChartGroup> SongList::get_song_entry(unsigned int entry) const
{
    if (!is_directory(entry))
        return std::get<std::shared_ptr<otoworm::ChartGroup>>(m_children_[entry].data);

    return nullptr;
}

std::string SongList::get_entry_title(unsigned int entry)
{
    if (entry >= m_children_.size())
        return "";

    if (is_directory(entry))
        return m_children_[entry].entry_name;

    auto song = std::get<std::shared_ptr<otoworm::ChartGroup>>(m_children_[entry].data);
    return song ? song->title : "<no song>";
}

unsigned int SongList::get_num_entries() const
{
    return m_children_.size();
}

bool SongList::has_parent_directory() const
{
    return m_parent_ != nullptr;
}

SongList* SongList::get_parent_directory() const
{
    return m_parent_;
}

void SongList::sort_by_fn(std::function<bool(const ListEntry&, const ListEntry&)> fn)
{
    std::ranges::stable_sort(m_children_, [&](const ListEntry& a, const ListEntry& b) {
        const bool a_is_dir = std::holds_alternative<std::shared_ptr<SongList>>(a.data);
        const bool b_is_dir = std::holds_alternative<std::shared_ptr<SongList>>(b.data);

        if (a_is_dir != b_is_dir)
            return a_is_dir;

        if (a_is_dir)
            return a.entry_name < b.entry_name;

        return fn(a, b);
    });
}

void SongList::sort_by(ESortCriteria criteria)
{
    switch (criteria) {
    case SORT_TITLE:
        sort_by_fn([](const ListEntry& a, const ListEntry& b) {
            auto chart_a = std::get<std::shared_ptr<otoworm::ChartGroup>>(a.data);
            auto chart_b = std::get<std::shared_ptr<otoworm::ChartGroup>>(b.data);
            return chart_a->title < chart_b->title;
        });
        break;
    case SORT_AUTHOR:
        sort_by_fn([](const ListEntry& a, const ListEntry& b) {
            auto chart_a = std::get<std::shared_ptr<otoworm::ChartGroup>>(a.data);
            auto chart_b = std::get<std::shared_ptr<otoworm::ChartGroup>>(b.data);
            return chart_a->artist < chart_b->artist;
        });
        break;
    case SORT_LENGTH:
        sort_by_fn([](const ListEntry& a, const ListEntry& b) {
            auto duration = [](const std::shared_ptr<otoworm::ChartGroup>& chart_group) {
                auto chart = chart_group->get_chart(0);
                return chart ? chart->duration : 0.0;
            };

            auto chart_a = std::get<std::shared_ptr<otoworm::ChartGroup>>(a.data);
            auto chart_b = std::get<std::shared_ptr<otoworm::ChartGroup>>(b.data);
            return duration(chart_a) < duration(chart_b);
        });
        break;
    case SORT_MINLEVEL:
        sort_by_fn([](const ListEntry& a, const ListEntry& b) {
            auto min_level = [](const std::shared_ptr<otoworm::ChartGroup>& chart_group) {
                long long level = 10000000;
                for (auto chart : chart_group->charts)
                    level = std::min(level, chart->level);
                return level;
            };

            auto chart_a = std::get<std::shared_ptr<otoworm::ChartGroup>>(a.data);
            auto chart_b = std::get<std::shared_ptr<otoworm::ChartGroup>>(b.data);
            return min_level(chart_a) < min_level(chart_b);
        });
        break;
    case SORT_MAXLEVEL:
        sort_by_fn([](const ListEntry& a, const ListEntry& b) {
            auto max_level = [](const std::shared_ptr<otoworm::ChartGroup>& chart_group) {
                long long level = -10000000;
                for (auto chart : chart_group->charts)
                    level = std::max(level, chart->level);
                return level;
            };

            auto chart_a = std::get<std::shared_ptr<otoworm::ChartGroup>>(a.data);
            auto chart_b = std::get<std::shared_ptr<otoworm::ChartGroup>>(b.data);
            return max_level(chart_a) < max_level(chart_b);
        });
        break;
    default:
        break;
    }

    for (auto&& child : m_children_) {
        if (std::holds_alternative<std::shared_ptr<SongList>>(child.data))
            std::get<std::shared_ptr<SongList>>(child.data)->sort_by(criteria);
    }
}
