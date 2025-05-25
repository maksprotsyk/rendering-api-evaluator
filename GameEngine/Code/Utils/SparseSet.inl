#pragma once

#include "SparseSet.h"

namespace Engine::Utils
{
	//////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    bool SparseSet<ElemType, IDType>::addElement(IDType entity, const ElemType& element)
    {
        if (isPresent(entity))
        {
            return false;
        }

        if (m_sparse.size() <= entity)
        {
            m_sparse.resize(entity + 1, -1);
        }

        m_sparse[entity] = m_dense.size();
        m_dense.push_back(element);
        m_denseEntities.push_back(entity);

        return true;
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    bool SparseSet<ElemType, IDType>::addElement(IDType entity, ElemType&& element)
    {
        if (isPresent(entity))
        {
            return false;
        }

        if (m_sparse.size() <= entity)
        {
            m_sparse.resize(entity + 1, -1);
        }

        m_sparse[entity] = m_dense.size();
        m_dense.emplace_back(std::move(element));
        m_denseEntities.push_back(entity);

        return true;
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    ElemType& SparseSet<ElemType, IDType>::getElement(IDType entity)
    {
        return m_dense[m_sparse[entity]];
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    const ElemType& SparseSet<ElemType, IDType>::getElement(IDType entity) const
    {
        return m_dense[m_sparse[entity]];
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    bool SparseSet<ElemType, IDType>::removeElement(IDType entity)
    {
        if (!isPresent(entity))
        {
            return false;
        }

        int denseIndex = m_sparse[entity];
        int lastDenseIndex = m_dense.size() - 1;

        m_dense[denseIndex] = std::move(m_dense[lastDenseIndex]);
        m_denseEntities[denseIndex] = m_denseEntities[lastDenseIndex];

        m_sparse[m_denseEntities[denseIndex]] = denseIndex;

        m_dense.pop_back();
        m_denseEntities.pop_back();

        m_sparse[entity] = -1;

        while (!m_sparse.empty() && m_sparse[m_sparse.size() - 1] == -1)
        {
            m_sparse.pop_back();
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////

    template<typename ElemType, typename IDType>
    inline void SparseSet<ElemType, IDType>::clear()
    {
		SparseSetBase<IDType>::clear();
		m_dense.clear();
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    const std::vector<ElemType>& SparseSet<ElemType, IDType>::getElements() const
    {
        return m_dense;
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename ElemType, typename IDType>
    std::vector<ElemType>& SparseSet<ElemType, IDType>::getElements()
    {
        return m_dense;
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename IDType>
    bool SparseSetBase<IDType>::isPresent(IDType entity) const
    {
        return m_sparse.size() > entity && m_sparse[entity] != -1;
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename IDType>
    size_t SparseSetBase<IDType>::size() const
    {
        return m_denseEntities.size();
    }

    //////////////////////////////////////////////////////////////////////////

    template<typename IDType>
    bool SparseSetBase<IDType>::removeElement(IDType entity)
    {
        if (!isPresent(entity))
        {
            return false;
        }

        int denseIndex = m_sparse[entity];
        int lastDenseIndex = m_denseEntities.size() - 1;

        m_denseEntities[denseIndex] = m_denseEntities[lastDenseIndex];
        m_sparse[m_denseEntities[denseIndex]] = denseIndex;
        m_denseEntities.pop_back();
        m_sparse[entity] = -1;

        while (!m_sparse.empty() && m_sparse[m_sparse.size() - 1] == -1)
        {
            m_sparse.pop_back();
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////

    template<typename IDType>
    inline void SparseSetBase<IDType>::clear()
	{
		m_sparse.clear();
		m_denseEntities.clear();
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename IDType>
    const std::vector<IDType>& SparseSetBase<IDType>::getIds() const
    {
        return m_denseEntities;
    }

    //////////////////////////////////////////////////////////////////////////
}

