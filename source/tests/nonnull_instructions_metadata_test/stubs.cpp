int global_var= 0;

extern "C" int* RetRef()
{
	return &global_var;
}

extern "C" int* RetPtr()
{
	return &global_var;
}

