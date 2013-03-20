#define FTDI_DEVICE_DRIVER_LOW_CHECK(r) \
{\
	do { \
	int res = (r);\
	if (0 != res)\
		{\
		return res;\
		}\
	} while (0);\
}

//void check_return_value(int value)
//{
//	const std::string what_happened =
//		"Zero pointer (function/object/method) detected for parameter '" + parameter_name + "'!";
//	if (0 != value)
//	{
//		throw std::invalid_argument::
//	}
//}
