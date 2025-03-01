int go(const char* c) {
	return 1;
}

const char* name = "Hello";
int main();
void _start() __attribute__((export_name("_start"))) {
	main();
}

int main() __attribute__((export_name("main"))) {
	if (go(name)) 
		return 1;
	else
	 	return 2;
}
