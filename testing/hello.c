long go_far(int a, int b, long c, long d, double e) {
	return a + b + c + d;
}

int go(const char* c) {
	return 1;
}

const char* name = "Hello";
int main();
void _start() __attribute__((export_name("_start"))) {
	go_far(1, 1, 3, 3, 1.0f);
	main();
}

int main() __attribute__((export_name("main"))) {
	if (go(name)) 
		return 1;
	else
	 	return 2;
}
