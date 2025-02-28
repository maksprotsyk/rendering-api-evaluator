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
        std::vector<int> m_sparse; // Maps entity ID to index in dense array
        std::vector<IDType> m_denseEntities; // Maps dense index back to entity ID
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

        using SparseSetBase<IDType>::m_sparse;
        using SparseSetBase<IDType>::m_denseEntities;

        std::vector<ElemType> m_dense; // Stores the actual components
    };
}

#include "SparseSet.inl"