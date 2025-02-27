#pragma once
#include <cstdint>
#include <concepts>
#include <vector>
#include <iterator>

namespace Engine::Utils
{
    template <typename IDType>
    class SparseSetBase
    {
    public:
        const std::vector<IDType>& getIds() const;
        bool isPresent(IDType entity) const;
        size_t size() const;
        virtual bool removeElement(IDType id);
        virtual void clear();

    protected:
        std::vector<int> _sparse; // Maps entity ID to index in dense array
        std::vector<IDType> _denseEntities; // Maps dense index back to entity ID
    };

    template <typename ElemType, typename IDType>
    class SparseSet: public SparseSetBase<IDType> 
    {
    public:
        bool addElement(IDType entity, const ElemType& component);
        bool addElement(IDType entity, ElemType&& component);

        ElemType& getElement(IDType entity);
        const ElemType& getElement(IDType entity) const;

        bool removeElement(IDType entity) override;
        void clear() override;

        const std::vector<ElemType>& getElements() const;
        std::vector<ElemType>& getElements();

        using SparseSetBase<IDType>::isPresent;
        using SparseSetBase<IDType>::getIds;
        using SparseSetBase<IDType>::size;

    private:

        using SparseSetBase<IDType>::_sparse;
        using SparseSetBase<IDType>::_denseEntities;

        std::vector<ElemType> _dense; // Stores the actual components
    };
}

namespace Engine::Utils
{
    template <typename ElemType, typename IDType>
    bool SparseSet<ElemType, IDType>::addElement(IDType entity, const ElemType& element)
    {
        if (isPresent(entity))
        {
            return false;
        }

        if (_sparse.size() <= entity)
        {
            _sparse.resize(entity + 1, -1);
        }

        _sparse[entity] = _dense.size();
        _dense.push_back(element);
        _denseEntities.push_back(entity);

        return true;
    }

    template <typename ElemType, typename IDType>
    bool SparseSet<ElemType, IDType>::addElement(IDType entity, ElemType&& element)
    {
        if (isPresent(entity))
        {
            return false;
        }

        if (_sparse.size() <= entity)
        {
            _sparse.resize(entity + 1, -1);
        }

        _sparse[entity] = _dense.size();
        _dense.emplace_back(std::move(element));
        _denseEntities.push_back(entity);

        return true;
    }

    template <typename ElemType, typename IDType>
    ElemType& SparseSet<ElemType, IDType>::getElement(IDType entity)
    {
        return _dense[_sparse[entity]];
    }

    template <typename ElemType, typename IDType>
    const ElemType& SparseSet<ElemType, IDType>::getElement(IDType entity) const
    {
        return _dense[_sparse[entity]];
    }

    template <typename ElemType, typename IDType>
    bool SparseSet<ElemType, IDType>::removeElement(IDType entity)
    {
        if (!isPresent(entity))
        {
            return false;
        }

        int denseIndex = _sparse[entity];
        int lastDenseIndex = _dense.size() - 1;

        // Swap the component to delete with the last one in the dense array
        _dense[denseIndex] = std::move(_dense[lastDenseIndex]);
        _denseEntities[denseIndex] = _denseEntities[lastDenseIndex];

        // Update the sparse array to reflect the new index of the swapped entity
        _sparse[_denseEntities[denseIndex]] = denseIndex;

        // Remove the last element in the dense array
        _dense.pop_back();
        _denseEntities.pop_back();

        // Mark the sparse array for the deleted entity as invalid
        _sparse[entity] = -1;

        while (!_sparse.empty() && _sparse[_sparse.size() - 1] == -1)
        {
            _sparse.pop_back();
        }

        return true;
    }

    template<typename ElemType, typename IDType>
    inline void SparseSet<ElemType, IDType>::clear()
    {
		SparseSetBase<IDType>::clear();
		_dense.clear();
    }

    template <typename ElemType, typename IDType>
    const std::vector<ElemType>& SparseSet<ElemType, IDType>::getElements() const
    {
        return _dense;
    }

    template <typename ElemType, typename IDType>
    std::vector<ElemType>& SparseSet<ElemType, IDType>::getElements()
    {
        return _dense;
    }

    template <typename IDType>
    bool SparseSetBase<IDType>::isPresent(IDType entity) const
    {
        return _sparse.size() > entity && _sparse[entity] != -1;
    }

    template <typename IDType>
    size_t SparseSetBase<IDType>::size() const
    {
        return _denseEntities.size();
    }

    template<typename IDType>
    bool SparseSetBase<IDType>::removeElement(IDType entity)
    {
        if (!isPresent(entity))
        {
            return false;
        }

        int denseIndex = _sparse[entity];
        int lastDenseIndex = _denseEntities.size() - 1;

        _denseEntities[denseIndex] = _denseEntities[lastDenseIndex];
        _sparse[_denseEntities[denseIndex]] = denseIndex;
        _denseEntities.pop_back();
        _sparse[entity] = -1;

        while (!_sparse.empty() && _sparse[_sparse.size() - 1] == -1)
        {
            _sparse.pop_back();
        }

        return true;
    }

    template<typename IDType>
    inline void SparseSetBase<IDType>::clear()
	{
		_sparse.clear();
		_denseEntities.clear();
    }

    template <typename IDType>
    const std::vector<IDType>& SparseSetBase<IDType>::getIds() const
    {
        return _denseEntities;
    }
}

