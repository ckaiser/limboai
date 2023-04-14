/* behavior_tree_data.h */

#ifndef BEHAVIOR_TREE_DATA_H
#define BEHAVIOR_TREE_DATA_H

#include "core/object/object.h"
#include "modules/limboai/bt/bt_task.h"

class BehaviorTreeData {
public:
	struct TaskData {
		int id = 0;
		String name;
		int num_children = 0;
		int status = 0;
		double elapsed_time = 0.0;
		String type_name;
		// String script_path;
		// String resource_path;

		TaskData(int p_id, const String &p_name, int p_num_children, int p_status, double p_elapsed_time, const String &p_type_name) {
			id = p_id;
			name = p_name;
			num_children = p_num_children;
			status = p_status;
			elapsed_time = p_elapsed_time;
			type_name = p_type_name;
		}

		TaskData() {}
	};

	List<TaskData> tasks;

	void serialize(Array &p_arr);
	void deserialize(const Array &p_arr);

	BehaviorTreeData(const Ref<BTTask> &p_instance);
	BehaviorTreeData() {}
};

#endif // BEHAVIOR_TREE_DATA