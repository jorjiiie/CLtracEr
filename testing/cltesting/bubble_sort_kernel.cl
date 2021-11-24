kernel void bubble_sort(
	global int* arr,
	global int* offset
	global int* flag)
{
	// if offset then we do the other way
	// essentially we launch kernels until there's no flag
	int id = get_global_id(0);

	// compare 2*id and 2*id+1 if offset = 0
	// else compare 2*id + 1 and 2*id + 2

	if (*offset) {
		if (arr[2*id+1] > arr[2*id+2]) {
			int tmp = arr[2*id+1];
			arr[2*id+1] = arr[2*id+2];
			arr[2*id+2]=tmp;
			flag|=1;
		}
	} else {
		if (arr[2*id] > arr[2*id+1]) {
			int tmp = arr[2*id];
			arr[2*id] = arr[2*id+1];
			arr[2*id+1] = tmp;
			flag|=1;
		}
	}
}