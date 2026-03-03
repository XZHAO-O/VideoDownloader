// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// c++ standard library
#include <type_traits>

// Project internal headers
#include "base_dao.h"

namespace nexusdl::database {

	template<typename T>
	concept HasInstance = requires { { T::instance() } -> std::same_as<T&>; };

	template<typename Entity, typename DAO>
	class BaseService
	{
		// 确保 DAO 是从 BaseDAO<DAO, Entity> 派生的 CRTP 类
		static_assert(std::is_base_of_v<BaseDAO<DAO, Entity>, DAO>,
			"DAO must derive from BaseDAO<DAO, Entity>");
		// 确保 DAO 提供一个静态实例方法
		static_assert(HasInstance<DAO>,
			"DAO must provide a static instance() method returning DAO&");

	public:
		virtual ~BaseService() = default;

		// 禁用拷贝/移动
		BaseService(const BaseService&) = delete;
		BaseService& operator=(const BaseService&) = delete;
		BaseService(BaseService&&) = delete;
		BaseService& operator=(BaseService&&) = delete;

		// ---------- CRUD 操作：转发给 DAO ----------
		std::expected<void, DatabaseError> insert(const Entity& entity)
		{
			return m_dao.insert(entity);
		}

		std::expected<void, DatabaseError> updateById(const Entity& entity)
		{
			return m_dao.updateById(entity);
		}

		std::expected<void, DatabaseError> deleteById(const QVariant& id)
		{
			return m_dao.deleteById(id);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectById(const QVariant& id)
		{
			return m_dao.selectById(id);
		}

		std::expected<std::vector<Entity>, DatabaseError> selectList(const QueryWrapper<Entity>& wrapper)
		{
			return m_dao.selectList(wrapper);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectOne(const QueryWrapper<Entity>& wrapper)
		{
			return m_dao.selectOne(wrapper);
		}

		std::expected<long, DatabaseError> selectCount(const QueryWrapper<Entity>& wrapper)
		{
			return m_dao.selectCount(wrapper);
		}

		std::expected<void, DatabaseError> beginTransaction()
		{
			return m_dao.beginTransaction();
		}

		std::expected<void, DatabaseError> commitTransaction()
		{
			return m_dao.commitTransaction();
		}

		std::expected<void, DatabaseError> rollbackTransaction()
		{
			return m_dao.rollbackTransaction();
		}

	protected:
		explicit BaseService() : m_dao{ DAO::instance() } {}

		DAO& m_dao;
	};

} // namespace nexusdl::database