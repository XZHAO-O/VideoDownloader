#ifndef ORDEREDQHASH_H
#define ORDEREDQHASH_H

#include <QHash>
#include <list>
#include <utility>
#include <ranges>
#include <concepts>

QT_BEGIN_NAMESPACE

template<typename K, typename Key>
concept CompatibleKey = std::convertible_to<K, Key>;

template<typename V, typename T>
concept CompatibleValue = std::convertible_to<V, T>;

template<typename Container, typename Key>
concept KeyCompatibleContainer = std::ranges::range<Container> && CompatibleKey<std::ranges::range_value_t<Container>, Key>;

template<typename It, typename Key, typename T>
concept InputIteratorToPair = std::input_iterator<It> && requires(It it)
{
	{ it->first } -> CompatibleKey<Key>;
	{ it->second } -> CompatibleValue<T>;
};

template<typename F, typename Key, typename T>
concept KeyValuePredicate = requires(F f, const Key & k, const T & v) {
	{ f(k, v) } -> std::convertible_to<bool>;
};

template<typename F, typename Key, typename T>
concept ValuePredicate = requires(F f, const std::pair<Key, T>&pair) {
	{ f(pair) } -> std::convertible_to<bool>;
};

template <typename Key, typename T>
class OrderedQHash
{
private:
	using ListType = std::list<std::pair<Key, T>>;
	using ListIterator = typename ListType::iterator;
	using ConstListIterator = typename ListType::const_iterator;
	using HashType = QHash<Key, ListIterator>;

public:
	using key_type = Key;
	using mapped_type = T;
	using value_type = typename ListType::value_type;
	using size_type = typename ListType::size_type;
	using difference_type = typename ListType::difference_type;

	class iterator
	{
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		//using value_type = typename ListType::value_type;
		using difference_type = typename ListType::difference_type;
		using pointer = T*;
		using reference = T&;

		constexpr iterator() noexcept = default;
		explicit iterator(ListIterator it) noexcept : m_current(it) {}

		reference operator*() const noexcept { return value(); }
		pointer operator->() const noexcept { return &(value()); }

		iterator& operator++() noexcept
		{
			++m_current;
			return *this;
		}
		iterator operator++(int) noexcept
		{
			iterator tmp = *this;
			++(*this);
			return tmp;
		}

		iterator& operator--() noexcept
		{
			--m_current;
			return *this;
		}
		iterator operator--(int) noexcept
		{
			iterator tmp = *this;
			--(*this);
			return tmp;
		}

		bool operator==(const iterator& other) const noexcept { return m_current == other.m_current; }
		bool operator!=(const iterator& other) const noexcept { return m_current != other.m_current; }

		// 获取当前元素的键
		const Key& key() const noexcept { return (*m_current).first; }

		// 获取当前元素的值
		T& value() const noexcept { return (*m_current).second; }

		ListIterator listIterator() const noexcept { return m_current; }

	private:
		ListIterator m_current;
	};

	class const_iterator
	{
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		//using value_type = const typename ListType::value_type;
		using difference_type = typename ListType::difference_type;
		using pointer = const T*;
		using reference = const T&;

		constexpr const_iterator() noexcept = default;
		const_iterator(ConstListIterator it) noexcept : m_current(it) {}
		const_iterator(const iterator& other) noexcept : m_current(other.listIterator()) {}

		reference operator*() const noexcept { return value(); }
		pointer operator->() const noexcept { return &(value()); }

		const_iterator& operator++() noexcept
		{
			++m_current;
			return *this;
		}
		const_iterator operator++(int) noexcept
		{
			const_iterator tmp = *this;
			++(*this);
			return tmp;
		}

		const_iterator& operator--() noexcept
		{
			--m_current;
			return *this;
		}
		const_iterator operator--(int) noexcept
		{
			const_iterator tmp = *this;
			--(*this);
			return tmp;
		}

		bool operator==(const const_iterator& other) const noexcept { return m_current == other.m_current; }
		bool operator!=(const const_iterator& other) const noexcept { return m_current != other.m_current; }

		// 获取当前元素的键
		const Key& key() const noexcept { return (*m_current).first; }

		// 获取当前元素的值
		const T& value() const noexcept { return (*m_current).second; }

		ConstListIterator listIterator() const noexcept { return m_current; }

	private:
		ConstListIterator m_current;
	};

	// 构造函数
	OrderedQHash() = default;

	OrderedQHash(std::initializer_list<value_type> list)
	{
		reserve(list.size());  // 预先分配空间
		for (const auto& pair : list)
		{
			insert(pair.first, pair.second);
		}
	}

	OrderedQHash(const OrderedQHash& other)
		: m_list(other.m_list)
	{
		rebuildHashFromList();
	}

	OrderedQHash(OrderedQHash&& other) noexcept
		: OrderedQHash()  // 委托默认构造
	{
		swap(other);
	}

	// 赋值运算符
	OrderedQHash& operator=(const OrderedQHash& other)
	{
		if (this != &other)
		{
			m_list = other.m_list;
			rebuildHashFromList();
		}
		return *this;
	}

	OrderedQHash& operator=(OrderedQHash&& other) noexcept
	{
		if (this != &other)
		{
			swap(other);    // 交换内容，让 other 负责清理旧资源
		}
		return *this;
	}

	OrderedQHash& operator=(std::initializer_list<value_type> list)
	{
		clear();
		reserve(list.size());  // 预先分配空间
		for (const auto& pair : list)
		{
			insert(pair.first, pair.second);
		}
		return *this;
	}

	// 容量相关
	bool isEmpty() const noexcept { return m_list.empty(); }
	size_type size() const noexcept { return m_list.size(); }
	size_type count() const noexcept { return size(); }

	size_type capacity() const noexcept { return m_hash.capacity(); }
	void reserve(size_type size) { m_hash.reserve(size); }
	void rehash(size_type count) { reserve(count); }
	void squeeze() { m_hash.squeeze(); }

	float load_factor() const noexcept { return m_hash.load_factor(); }
	static float max_load_factor() noexcept { return QHash<Key, ListType>::max_load_factor(); };
	size_type bucket_count() const noexcept { return m_hash.bucket_count(); }
	static size_type max_bucket_count() noexcept { return QHash<Key, ListType>::max_bucket_count(); }

	// 元素访问
	T& operator[](const Key& key)
	{
		auto it = find(key);
		if (it != end())
		{
			return it.value();
		}
		return (emplace(key, T{})).value();
	}

	const T operator[](const Key& key) const noexcept
	{
		return value(key);
	}

	T value(const Key& key) const noexcept
	{
		auto it = find(key);
		if (it != end())
		{
			return it.value();
		}
		return T{};
	}

	T value(const Key& key, const T& defaultValue) const noexcept
	{
		auto it = find(key);
		return it != end() ? it.value() : defaultValue;
	}

	// 修改器
	void clear()
	{
		m_list.clear();
		m_hash.clear();
	}

	bool remove(const Key& key)
	{
		if (isEmpty()) return false;

		auto hashIt = m_hash.find(key);
		if (hashIt != m_hash.end())
		{
			m_list.erase(hashIt.value());
			m_hash.erase(hashIt);
			return true;
		}
		return false;
	}

	T take(const Key& key)
	{
		if (isEmpty()) return T{};

		auto hashIt = m_hash.find(key);
		if (hashIt != m_hash.end())
		{
			T value = std::move(hashIt.value()->second);
			m_list.erase(hashIt.value());
			m_hash.erase(hashIt);
			return value;
		}
		return T{};
	}

	// 通过迭代器删除元素
	iterator erase(const_iterator it)
	{
		if (it == end()) return end();

		auto listIt = it.listIterator();
		m_hash.remove((*listIt).first);
		auto nextIt = m_list.erase(listIt);
		return iterator(nextIt);
	}

	// 通过谓词删除元素
	template<typename Predicate>
		requires ValuePredicate<Predicate, Key, T>
	size_type eraseIf(Predicate pred)
	{
		if (isEmpty()) return 0;
		size_type removedCount = 0;
		auto it = m_list.begin();
		while (it != m_list.end())
		{
			if (pred(*it))
			{
				m_hash.remove(it->first);
				it = m_list.erase(it);
				++removedCount;
			}
			else
			{
				++it;
			}
		}
		return removedCount;
	}

	// 通过键值对谓词删除元素
	template<typename Predicate>
		requires KeyValuePredicate<Predicate, Key, T>
	size_type removeIf(Predicate pred)
	{
		return eraseIf([&pred](const value_type& pair) -> bool {
			return pred(pair.first, pair.second);
			});
	}

	// 插入操作 - 使用完美转发，添加概念约束
	template<typename K, typename V>
		requires CompatibleKey<K, Key>&& CompatibleValue<V, T>
	iterator insert(K&& key, V&& value)
	{
		return emplace(std::forward<K>(key), std::forward<V>(value));
	}

	void insert(std::initializer_list<std::pair<Key, T>> list)
	{
		if (list.size() == 0) return;

		reserve(size() + list.size());  // 预先分配空间
		for (const auto& pair : list)
		{
			insert(pair.first, pair.second);
		}
	}

	// 插入操作 - 使用完美转发，添加概念约束
	template<typename K, typename... Args>
		requires CompatibleKey<K, Key>&& std::constructible_from<T, Args...>
	iterator emplace(K&& key, Args&&... args)
	{
		auto hashIt = m_hash.find(key);
		if (hashIt != m_hash.end())
		{
			// 使用构造+交换，提供强异常安全保证
			T new_value(std::forward<Args>(args)...);
			std::swap(hashIt.value()->second, new_value);
			return iterator(hashIt.value());
		}

		// 插入新元素到列表末尾
		m_list.emplace_back(
			std::piecewise_construct,
			std::forward_as_tuple(std::forward<K>(key)),
			std::forward_as_tuple(std::forward<Args>(args)...)
		);
		auto listIt = --m_list.end();
		m_hash.insert(listIt->first, listIt);
		return iterator(listIt);
	}

	// 批量操作函数
	void insert(const OrderedQHash& other)
	{
		if (other.isEmpty()) return;

		reserve(size() + other.size());  // 预先分配空间
		for (auto it = other.begin(); it != other.end(); ++it)
		{
			insert(it.key(), it.value());
		}
	}

	void insert(const QHash<Key, T>& hash)
	{
		if (hash.isEmpty()) return;

		reserve(size() + hash.size());  // 预先分配空间
		for (auto it = hash.begin(); it != hash.end(); ++it)
		{
			insert(it.key(), it.value());
		}
	}

	template<typename InputIterator>
		requires InputIteratorToPair<InputIterator, Key, T>
	void insert(InputIterator first, InputIterator last)
	{
		// 对于随机访问迭代器，我们可以计算距离并预先分配空间
		if constexpr (std::is_same_v<typename std::iterator_traits<InputIterator>::iterator_category,
			std::random_access_iterator_tag>)
		{
			auto distance = std::distance(first, last);
			if (distance > 0)
			{
				reserve(size() + distance);  // 预先分配空间
			}
		}

		for (auto it = first; it != last; ++it)
		{
			insert(it->first, it->second);
		}
	}

	OrderedQHash& unite(const OrderedQHash& other)
	{
		insert(other);
		return *this;
	}

	OrderedQHash& unite(const QHash<Key, T>& hash)
	{
		insert(hash);
		return *this;
	}

	size_type remove(std::initializer_list<Key> keys)
	{
		if (isEmpty()) return 0;

		size_type count = 0;
		for (const auto& key : keys)
		{
			if (remove(key))
			{
				++count;
			}
		}
		return count;
	}

	template<typename Container>
		requires KeyCompatibleContainer<Container, Key>
	size_type remove(const Container& keys)
	{
		if (isEmpty()) return 0;

		size_type count = 0;
		for (const auto& key : keys)
		{
			if (remove(key))
			{
				++count;
			}
		}
		return count;
	}

	// 批量查找
	template<typename Container>
		requires KeyCompatibleContainer<Container, Key>
	bool contains(const Container& keys) const
	{
		if (isEmpty()) return false;

		for (const auto& key : keys)
		{
			if (!contains(key))
			{
				return false;
			}
		}
		return true;
	}

	// 批量取值
	QHash<Key, T> toHash() const
	{
		if (isEmpty()) return QHash<Key, T>();
		QHash<Key, T> result;
		result.reserve(size());  // 预先分配空间
		for (const auto& pair : m_list)
		{
			result.insert(pair.first, pair.second);
		}
		return result;
	}

	QList<std::pair<Key, T>> toList() const
	{
		if (isEmpty()) return QList<std::pair<Key, T>>();
		QList<std::pair<Key, T>> result;
		result.reserve(size());  // 预先分配空间
		for (const auto& pair : m_list)
		{
			result.append(pair);
		}
		return result;
	}

	// 过滤操作
	template<typename Predicate>
		requires KeyValuePredicate<Predicate, Key, T>
	OrderedQHash filtered(Predicate predicate) const
	{
		if (isEmpty()) return OrderedQHash();
		OrderedQHash result;

		// 预分配空间
		size_t estimatedSize = 0;
		for (const auto& pair : m_list)
		{
			if (predicate(pair.first, pair.second))
			{
				++estimatedSize;
			}
		}
		result.reserve(estimatedSize);

		// 复制满足条件的元素
		for (const auto& pair : m_list)
		{
			if (predicate(pair.first, pair.second))
			{
				result.insert(pair.first, pair.second);
			}
		}

		return result;
	}

	template<typename Predicate>
		requires KeyValuePredicate<Predicate, Key, T>
	void filter(Predicate predicate)
	{
		if (isEmpty()) return;
		auto it = m_list.begin();
		while (it != m_list.end())
		{
			if (!predicate(it->first, it->second))
			{
				// 直接删除不满足条件的元素
				m_hash.remove(it->first);
				it = m_list.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// 查找操作
	iterator find(const Key& key)
	{
		if (isEmpty()) return end();
		auto hashIt = m_hash.find(key);
		return hashIt != m_hash.end() ? iterator(hashIt.value()) : end();
	}

	const_iterator find(const Key& key) const noexcept
	{
		if (isEmpty()) return cend();
		auto hashIt = m_hash.find(key);
		return hashIt != m_hash.end() ? const_iterator(hashIt.value()) : end();
	}

	const_iterator constFind(const Key& key) const noexcept
	{
		return find(key);
	}

	bool contains(const Key& key) const noexcept
	{
		if (isEmpty()) return false;
		return m_hash.contains(key);
	}

	size_type count(const Key& key) const noexcept
	{
		return contains(key) ? 1 : 0;
	}

	// 迭代器
	iterator begin() noexcept { return iterator(m_list.begin()); }
	iterator end() noexcept { return iterator(m_list.end()); }
	const_iterator begin() const noexcept { return const_iterator(m_list.begin()); }
	const_iterator end() const noexcept { return const_iterator(m_list.end()); }
	const_iterator cbegin() const noexcept { return const_iterator(m_list.begin()); }
	const_iterator cend() const noexcept { return const_iterator(m_list.end()); }
	const_iterator constBegin() const noexcept { return const_iterator(m_list.begin()); }
	const_iterator constEnd() const noexcept { return const_iterator(m_list.end()); }

	// 键和值列表
	QList<Key> keys() const
	{
		if (isEmpty()) return QList<Key>();
		QList<Key> result;
		result.reserve(size());  // 预先分配空间
		for (const auto& pair : m_list)
		{
			result.append(pair.first);
		}
		return result;
	}

	QList<Key> keys(const T& value) const
	{
		if (isEmpty()) return QList<Key>();
		QList<Key> res;
		const_iterator i = begin();
		while (i != end())
		{
			if (i.value() == value)
				res.append(i.key());
			++i;
		}
		return res;
	}

	QList<T> values() const
	{
		if (isEmpty()) return QList<T>();
		QList<T> result;
		result.reserve(size());  // 预先分配空间
		for (const auto& pair : m_list)
		{
			result.append(pair.second);
		}
		return result;
	}

	// 顺序相关操作 - 使用完美转发，添加概念约束
	template<typename K, typename V>
		requires CompatibleKey<K, Key>&& CompatibleValue<V, T>
	iterator pushBack(K&& key, V&& value)
	{
		return insert(std::forward<K>(key), std::forward<V>(value));
	}

	// 顺序相关操作 - 前端插入，使用完美转发，添加概念约束
	template<typename K, typename V>
		requires CompatibleKey<K, Key>&& CompatibleValue<V, T>
	iterator pushFront(K&& key, V&& value)
	{
		auto hashIt = m_hash.find(key);

		// 先插入到列表头部
		m_list.emplace_front(std::forward<K>(key), std::forward<V>(value));
		auto listIt = m_list.begin();

		if (hashIt != m_hash.end())
		{
			// 如果键已存在，更新迭代器并删除旧位置
			m_list.erase(hashIt.value());
			hashIt.value() = listIt;
		}
		else
		{
			m_hash.insert(listIt->first, listIt);
		}

		return iterator(listIt);
	}

	T& first() { return m_list.front().second; }
	const T& first() const { return m_list.front().second; }
	T& last() { return m_list.back().second; }
	const T& last() const { return m_list.back().second; }

	const Key& firstKey() const { return m_list.front().first; }
	const Key& lastKey() const { return m_list.back().first; }

	// 交换
	void swap(OrderedQHash& other) noexcept
	{
		m_list.swap(other.m_list);
		m_hash.swap(other.m_hash);
	}

	// 比较操作
	bool operator==(const OrderedQHash& other) const noexcept
	{
		return m_list == other.m_list;
	}

	bool operator!=(const OrderedQHash& other) const noexcept
	{
		return m_list != other.m_list;
	}

	// 操作符重载
	OrderedQHash& operator+=(const OrderedQHash& other)
	{
		return unite(other);
	}

	OrderedQHash& operator+=(const QHash<Key, T>& hash)
	{
		return unite(hash);
	}

private:
	void rebuildHashFromList()
	{
		m_hash.clear();
		m_hash.reserve(m_list.size());

		for (auto it = m_list.begin(); it != m_list.end(); ++it)
		{
			m_hash[it->first] = it;  // 使用 operator[]
		}
	}

private:
	ListType m_list;
	HashType m_hash;
};

// 交换函数
template<typename Key, typename T>
void swap(OrderedQHash<Key, T>& lhs, OrderedQHash<Key, T>& rhs) noexcept
{
	lhs.swap(rhs);
}

QT_END_NAMESPACE

#endif // ORDEREDQHASH_H