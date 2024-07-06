#pragma once

#include <concepts>
#include <iterator>

namespace Utility::Algorithm
{
    /**
     * @brief Transforms a sequence of elements but is able to leave elements out.
     *
     * @tparam IteratorT The type of the iterators.
     * @tparam PredicateT The type of the transformer which also is responsible of assigning to the output iterator.
     * @param begin An iterator to the beginning of the input sequence.
     * @param end An iterator to the end of the input sequence.
     * @param out A back_insert_iterator to the beginning of the output sequence.
     * @param inserter A function taking the output iterator, an element and returns whether an item was updated.
     */
    template <typename InputIteratorT, typename ContainerT, typename TransformInserterT>
    requires std::
        predicate<TransformInserterT, std::back_insert_iterator<ContainerT>&, typename InputIteratorT::value_type>
        void transformIf(
            InputIteratorT begin,
            InputIteratorT const& end,
            std::back_insert_iterator<ContainerT> out,
            TransformInserterT const& inserter)
    {
        for (; begin != end; ++begin)
        {
            if (inserter(out, *begin))
                ++out;
        }
    }
}