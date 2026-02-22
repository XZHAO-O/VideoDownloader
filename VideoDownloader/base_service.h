// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// c++ standard library
#include <expected>
#include <memory>
#include <optional>
#include <vector>
#include <type_traits>

// Project internal headers
#include "base_dao.h"
#include "database_error.h"
#include "query_wrapper.h"

namespace nexusdl::database {

	template<typename Entity, typename DAO = BaseDAO<Entity>>
	class BaseService
	{
		static_assert(std::is_base_of_v<BaseDAO<Entity>, DAO>,
			"DAO must derive from BaseDAO<Entity>");

	public:
		using DAOPtr = std::shared_ptr<DAO>;

		explicit BaseService(DAOPtr dao)
			: m_dao{ std::move(dao) }
		{
		}

		virtual ~BaseService() = default;

		// 禁用拷贝/移动（可根据需要调整）
		BaseService(const BaseService&) = delete;
		BaseService& operator=(const BaseService&) = delete;
		BaseService(BaseService&&) = delete;
		BaseService& operator=(BaseService&&) = delete;

		// 获取具体的 DAO 实例，用于调用自定义方法
		DAO* dao() noexcept { return m_dao.get(); }
		const DAO* dao() const noexcept { return m_dao.get(); }

		// ---------- CRUD 操作：转发给 DAO ----------
		std::expected<void, DatabaseError> insert(const Entity& entity)
		{
			return m_dao->insert(entity);
		}

		std::expected<void, DatabaseError> updateById(const Entity& entity)
		{
			return m_dao->updateById(entity);
		}

		std::expected<void, DatabaseError> deleteById(const QVariant& id)
		{
			return m_dao->deleteById(id);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectById(const QVariant& id)
		{
			return m_dao->selectById(id);
		}

		std::expected<std::vector<Entity>, DatabaseError> selectList(const QueryWrapper<Entity>& wrapper)
		{
			return m_dao->selectList(wrapper);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectOne(const QueryWrapper<Entity>& wrapper)
		{
			return m_dao->selectOne(wrapper);
		}

		std::expected<long, DatabaseError> selectCount(const QueryWrapper<Entity>& wrapper)
		{
			return m_dao->selectCount(wrapper);
		}

		// ---------- 事务支持 ----------
		std::expected<void, DatabaseError> beginTransaction()
		{
			return m_dao->m_db->beginTransaction();
		}

		std::expected<void, DatabaseError> commitTransaction()
		{
			return m_dao->m_db->commitTransaction();
		}

		std::expected<void, DatabaseError> rollbackTransaction()
		{
			return m_dao->m_db->rollbackTransaction();
		}

	protected:
		DAOPtr m_dao;
	};

} // namespace nexusdl::database