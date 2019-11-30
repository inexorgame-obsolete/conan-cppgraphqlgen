// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "TodaySchema.h"

#include <stack>

namespace graphql::today {

struct RequestState : service::RequestState
{
	RequestState(size_t id)
		: requestId(id)
	{
	}

	const size_t requestId;

	size_t appointmentsRequestId = 0;
	size_t tasksRequestId = 0;
	size_t unreadCountsRequestId = 0;

	size_t loadAppointmentsCount = 0;
	size_t loadTasksCount = 0;
	size_t loadUnreadCountsCount = 0;
};

class Appointment;
class Task;
class Folder;

class Query : public object::Query
{
public:
	using appointmentsLoader = std::function<std::vector<std::shared_ptr<Appointment>>()>;
	using tasksLoader = std::function<std::vector<std::shared_ptr<Task>>()>;
	using unreadCountsLoader = std::function<std::vector<std::shared_ptr<Folder>>()>;

	explicit Query(appointmentsLoader&& getAppointments, tasksLoader&& getTasks, unreadCountsLoader&& getUnreadCounts);

	service::FieldResult<std::shared_ptr<service::Object>> getNode(service::FieldParams&& params, response::IdType&& id) const override;
	service::FieldResult<std::shared_ptr<object::AppointmentConnection>> getAppointments(service::FieldParams&& params, std::optional<response::IntType>&& first, std::optional<response::Value>&& after, std::optional<response::IntType>&& last, std::optional<response::Value>&& before) const override;
	service::FieldResult<std::shared_ptr<object::TaskConnection>> getTasks(service::FieldParams&& params, std::optional<response::IntType>&& first, std::optional<response::Value>&& after, std::optional<response::IntType>&& last, std::optional<response::Value>&& before) const override;
	service::FieldResult<std::shared_ptr<object::FolderConnection>> getUnreadCounts(service::FieldParams&& params, std::optional<response::IntType>&& first, std::optional<response::Value>&& after, std::optional<response::IntType>&& last, std::optional<response::Value>&& before) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Appointment>>> getAppointmentsById(service::FieldParams&& params, std::vector<response::IdType>&& ids) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Task>>> getTasksById(service::FieldParams&& params, std::vector<response::IdType>&& ids) const override;
	service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> getUnreadCountsById(service::FieldParams&& params, std::vector<response::IdType>&& ids) const override;
	service::FieldResult<std::shared_ptr<object::NestedType>> getNested(service::FieldParams&& params) const override;

private:
	std::shared_ptr<Appointment> findAppointment(const service::FieldParams& params, const response::IdType& id) const;
	std::shared_ptr<Task> findTask(const service::FieldParams& params, const response::IdType& id) const;
	std::shared_ptr<Folder> findUnreadCount(const service::FieldParams& params, const response::IdType& id) const;

	// Lazy load the fields in each query
	void loadAppointments(const std::shared_ptr<service::RequestState>& state) const;
	void loadTasks(const std::shared_ptr<service::RequestState>& state) const;
	void loadUnreadCounts(const std::shared_ptr<service::RequestState>& state) const;

	mutable appointmentsLoader _getAppointments;
	mutable tasksLoader _getTasks;
	mutable unreadCountsLoader _getUnreadCounts;

	mutable std::vector<std::shared_ptr<Appointment>> _appointments;
	mutable std::vector<std::shared_ptr<Task>> _tasks;
	mutable std::vector<std::shared_ptr<Folder>> _unreadCounts;
};

class PageInfo : public object::PageInfo
{
public:
	explicit PageInfo(bool hasNextPage, bool hasPreviousPage)
		: _hasNextPage(hasNextPage)
		, _hasPreviousPage(hasPreviousPage)
	{
	}

	service::FieldResult<bool> getHasNextPage(service::FieldParams&&) const override
	{
		return _hasNextPage;
	}

	service::FieldResult<bool> getHasPreviousPage(service::FieldParams&&) const override
	{
		return _hasPreviousPage;
	}

private:
	const bool _hasNextPage;
	const bool _hasPreviousPage;
};

class Appointment : public object::Appointment
{
public:
	explicit Appointment(response::IdType&& id, std::string&& when, std::string&& subject, bool isNow);

	service::FieldResult<response::IdType> getId(service::FieldParams&&) const override
	{
		return _id;
	}

	service::FieldResult<std::optional<response::Value>> getWhen(service::FieldParams&&) const override
	{
		return std::make_optional<response::Value>(std::string(_when));
	}

	service::FieldResult<std::optional<response::StringType>> getSubject(service::FieldParams&&) const override
	{
		return std::make_optional<response::StringType>(_subject);
	}

	service::FieldResult<bool> getIsNow(service::FieldParams&&) const override
	{
		return _isNow;
	}

private:
	response::IdType _id;
	std::string _when;
	std::string _subject;
	bool _isNow;
};

class AppointmentEdge : public object::AppointmentEdge
{
public:
	explicit AppointmentEdge(std::shared_ptr<Appointment> appointment)
		: _appointment(std::move(appointment))
	{
	}

	service::FieldResult<std::shared_ptr<object::Appointment>> getNode(service::FieldParams&&) const override
	{
		return std::static_pointer_cast<object::Appointment>(_appointment);
	}

	service::FieldResult<response::Value> getCursor(service::FieldParams&& params) const override
	{
		return response::Value(service::Base64::toBase64(_appointment->getId(std::move(params)).get()));
	}

private:
	std::shared_ptr<Appointment> _appointment;
};

class AppointmentConnection : public object::AppointmentConnection
{
public:
	explicit AppointmentConnection(bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Appointment>> appointments)
		: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
		, _appointments(std::move(appointments))
	{
	}

	service::FieldResult<std::shared_ptr<object::PageInfo>> getPageInfo(service::FieldParams&&) const override
	{
		return _pageInfo;
	}

	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::AppointmentEdge>>>> getEdges(service::FieldParams&&) const override
	{
		auto result = std::make_optional<std::vector<std::shared_ptr<object::AppointmentEdge>>>(_appointments.size());

		std::transform(_appointments.cbegin(), _appointments.cend(), result->begin(),
			[](const std::shared_ptr<Appointment>& node)
		{
			return std::make_shared<AppointmentEdge>(node);
		});

		return { std::move(result) };
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Appointment>> _appointments;
};

class Task : public object::Task
{
public:
	explicit Task(response::IdType&& id, std::string&& title, bool isComplete);

	service::FieldResult<response::IdType> getId(service::FieldParams&&) const override
	{
		return _id;
	}

	service::FieldResult<std::optional<response::StringType>> getTitle(service::FieldParams&&) const override
	{
		return std::make_optional<response::StringType>(_title);
	}

	service::FieldResult<bool> getIsComplete(service::FieldParams&&) const override
	{
		return _isComplete;
	}

private:
	response::IdType _id;
	std::string _title;
	bool _isComplete;
	TaskState _state = TaskState::New;
};

class TaskEdge : public object::TaskEdge
{
public:
	explicit TaskEdge(std::shared_ptr<Task> task)
		: _task(std::move(task))
	{
	}

	service::FieldResult<std::shared_ptr<object::Task>> getNode(service::FieldParams&&) const override
	{
		return std::static_pointer_cast<object::Task>(_task);
	}

	service::FieldResult<response::Value> getCursor(service::FieldParams&& params) const override
	{
		return response::Value(service::Base64::toBase64(_task->getId(std::move(params)).get()));
	}

private:
	std::shared_ptr<Task> _task;
};

class TaskConnection : public object::TaskConnection
{
public:
	explicit TaskConnection(bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Task>> tasks)
		: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
		, _tasks(std::move(tasks))
	{
	}

	service::FieldResult<std::shared_ptr<object::PageInfo>> getPageInfo(service::FieldParams&&) const override
	{
		return _pageInfo;
	}

	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::TaskEdge>>>> getEdges(service::FieldParams&&) const override
	{
		auto result = std::make_optional<std::vector<std::shared_ptr<object::TaskEdge>>>(_tasks.size());

		std::transform(_tasks.cbegin(), _tasks.cend(), result->begin(),
			[](const std::shared_ptr<Task>& node)
		{
			return std::make_shared<TaskEdge>(node);
		});

		return { std::move(result) };
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Task>> _tasks;
};

class Folder : public object::Folder
{
public:
	explicit Folder(response::IdType&& id, std::string&& name, int unreadCount);

	service::FieldResult<response::IdType> getId(service::FieldParams&&) const override
	{
		return _id;
	}

	service::FieldResult<std::optional<response::StringType>> getName(service::FieldParams&&) const override
	{
		return std::make_optional<response::StringType>(_name);
	}

	service::FieldResult<int> getUnreadCount(service::FieldParams&&) const override
	{
		return _unreadCount;
	}

private:
	response::IdType _id;
	std::string _name;
	int _unreadCount;
};

class FolderEdge : public object::FolderEdge
{
public:
	explicit FolderEdge(std::shared_ptr<Folder> folder)
		: _folder(std::move(folder))
	{
	}

	service::FieldResult<std::shared_ptr<object::Folder>> getNode(service::FieldParams&&) const override
	{
		return std::static_pointer_cast<object::Folder>(_folder);
	}

	service::FieldResult<response::Value> getCursor(service::FieldParams&& params) const override
	{
		return response::Value(service::Base64::toBase64(_folder->getId(std::move(params)).get()));
	}

private:
	std::shared_ptr<Folder> _folder;
};

class FolderConnection : public object::FolderConnection
{
public:
	explicit FolderConnection(bool hasNextPage, bool hasPreviousPage, std::vector<std::shared_ptr<Folder>> folders)
		: _pageInfo(std::make_shared<PageInfo>(hasNextPage, hasPreviousPage))
		, _folders(std::move(folders))
	{
	}

	service::FieldResult<std::shared_ptr<object::PageInfo>> getPageInfo(service::FieldParams&&) const override
	{
		return _pageInfo;
	}

	service::FieldResult<std::optional<std::vector<std::shared_ptr<object::FolderEdge>>>> getEdges(service::FieldParams&&) const override
	{
		auto result = std::make_optional<std::vector<std::shared_ptr<object::FolderEdge>>>(_folders.size());

		std::transform(_folders.cbegin(), _folders.cend(), result->begin(),
			[](const std::shared_ptr<Folder>& node)
		{
			return std::make_shared<FolderEdge>(node);
		});

		return { std::move(result) };
	}

private:
	std::shared_ptr<PageInfo> _pageInfo;
	std::vector<std::shared_ptr<Folder>> _folders;
};

class CompleteTaskPayload : public object::CompleteTaskPayload
{
public:
	explicit CompleteTaskPayload(std::shared_ptr<Task> task, std::optional<response::StringType>&& clientMutationId)
		: _task(std::move(task))
		, _clientMutationId(std::move(clientMutationId))
	{
	}

	service::FieldResult<std::shared_ptr<object::Task>> getTask(service::FieldParams&&) const override
	{
		return std::static_pointer_cast<object::Task>(_task);
	}

	service::FieldResult<std::optional<response::StringType>> getClientMutationId(service::FieldParams&&) const override
	{
		return {
			_clientMutationId
				? std::make_optional<response::StringType>(*_clientMutationId)
				: std::nullopt
		};
	}

private:
	std::shared_ptr<Task> _task;
	std::optional<response::StringType> _clientMutationId;
};

class Mutation : public object::Mutation
{
public:
	using completeTaskMutation = std::function<std::shared_ptr<CompleteTaskPayload>(CompleteTaskInput&&)>;

	explicit Mutation(completeTaskMutation&& mutateCompleteTask);

	service::FieldResult<std::shared_ptr<object::CompleteTaskPayload>> applyCompleteTask(service::FieldParams&& params, CompleteTaskInput&& input) const override;

private:
	completeTaskMutation _mutateCompleteTask;
};

class Subscription : public object::Subscription
{
public:
	explicit Subscription() = default;

	service::FieldResult<std::shared_ptr<object::Appointment>> getNextAppointmentChange(service::FieldParams&&) const override
	{
		throw std::runtime_error("Unexpected call to getNextAppointmentChange");
	}

	service::FieldResult<std::shared_ptr<service::Object>> getNodeChange(service::FieldParams&&, response::IdType&&) const override
	{
		throw std::runtime_error("Unexpected call to getNodeChange");
	}
};

class NextAppointmentChange : public object::Subscription
{
public:
	using nextAppointmentChange = std::function<std::shared_ptr<Appointment>(const std::shared_ptr<service::RequestState>&)>;

	explicit NextAppointmentChange(nextAppointmentChange&& changeNextAppointment)
		: _changeNextAppointment(std::move(changeNextAppointment))
	{
	}

	service::FieldResult<std::shared_ptr<object::Appointment>> getNextAppointmentChange(service::FieldParams&& params) const override
	{
		return std::static_pointer_cast<object::Appointment>(_changeNextAppointment(params.state));
	}

	service::FieldResult<std::shared_ptr<service::Object>> getNodeChange(service::FieldParams&&, response::IdType&&) const override
	{
		throw std::runtime_error("Unexpected call to getNodeChange");
	}

private:
	nextAppointmentChange _changeNextAppointment;
};

class NodeChange : public object::Subscription
{
public:
	using nodeChange = std::function<std::shared_ptr<service::Object>(const std::shared_ptr<service::RequestState>&, response::IdType&&)>;

	explicit NodeChange(nodeChange&& changeNode)
		: _changeNode(std::move(changeNode))
	{
	}

	service::FieldResult<std::shared_ptr<object::Appointment>> getNextAppointmentChange(service::FieldParams&&) const override
	{
		throw std::runtime_error("Unexpected call to getNextAppointmentChange");
	}

	service::FieldResult<std::shared_ptr<service::Object>> getNodeChange(service::FieldParams&& params, response::IdType&& idArg) const override
	{
		return std::static_pointer_cast<service::Object>(_changeNode(params.state, std::move(idArg)));
	}

private:
	nodeChange _changeNode;
};

struct CapturedParams
{
	// Copied in the constructor
	const response::Value operationDirectives;
	const response::Value fragmentDefinitionDirectives;
	const response::Value fragmentSpreadDirectives;
	const response::Value inlineFragmentDirectives;

	// Moved in the constructor
	const response::Value fieldDirectives;
};

class NestedType : public object::NestedType
{
public:
	explicit NestedType(service::FieldParams&& params, int depth);

	service::FieldResult<response::IntType> getDepth(service::FieldParams&& params) const override;
	service::FieldResult<std::shared_ptr<object::NestedType>> getNested(service::FieldParams&& params) const override;

	static std::stack<CapturedParams> getCapturedParams();

private:
	static std::stack<CapturedParams> _capturedParams;

	// Initialized in the constructor
	const int depth;
};

} /* namespace graphql::today */
