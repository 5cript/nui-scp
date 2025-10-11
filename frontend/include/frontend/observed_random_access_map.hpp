#pragma once

#include <nui/event_system/observed_value.hpp>

#include <deque>
#include <stdexcept>

template <typename KeyT, typename ValueT, template <typename, typename> typename MapT>
class ObservedRandomAccessMap
{
  public:
    ObservedRandomAccessMap() = default;

    void insert(KeyT const& key, ValueT const& value)
    {
        if (keyToIndexMap_.find(key) != keyToIndexMap_.end())
            throw std::invalid_argument("Key already exists in ObservedRandomAccessMap");

        observedValues_.push_back(value);
        keyToIndexMap_[key] = observedValues_.value().size() - 1;
    }

    void insert(KeyT const& key, ValueT&& value)
    {
        if (keyToIndexMap_.find(key) != keyToIndexMap_.end())
            throw std::invalid_argument("Key already exists in ObservedRandomAccessMap");

        observedValues_.push_back(std::move(value));
        keyToIndexMap_[key] = observedValues_.value().size() - 1;
    }

    void pop_back()
    {
        if (observedValues_.value().empty())
            return;

        keyToIndexMap_.erase(observedValues_.value().back().key());
        observedValues_.pop_back();
    }

    void pop_front()
    {
        if (observedValues_.empty())
            return;
        observedValues_.pop_front();

        // TODO: be more clever here:
        rebuildIndexMap();
    }

    void erase(KeyT const& key)
    {
        auto it = keyToIndexMap_.find(key);
        if (it == keyToIndexMap_.end())
            throw std::out_of_range("Key not found in ObservedRandomAccessMap");

        observedValues_.erase(observedValues_.begin() + it->second);

        // TODO: be more clever here:
        rebuildIndexMap();
    }

    Nui::Observed<std::deque<ValueT>>& observedValues()
    {
        return observedValues_;
    }

    ValueT* at(KeyT const& key)
    {
        auto it = keyToIndexMap_.find(key);
        if (it == keyToIndexMap_.end())
            return nullptr;

        return &observedValues_.value().at(it->second);
    }

    template <typename FunctionT>
    void modify(KeyT const& key, FunctionT const& modifier)
    {
        auto it = keyToIndexMap_.find(key);
        if (it == keyToIndexMap_.end())
            throw std::out_of_range("Key not found in ObservedRandomAccessMap");

        modifier(observedValues_[it->second]);
    }

  private:
    void rebuildIndexMap()
    {
        keyToIndexMap_.clear();
        for (std::size_t i = 0; i < observedValues_.value().size(); ++i)
        {
            keyToIndexMap_[observedValues_.value().at(i).key()] = i;
        }
    }

  private:
    Nui::Observed<std::deque<ValueT>> observedValues_;
    MapT<KeyT, std::size_t> keyToIndexMap_;
};