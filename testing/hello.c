extern int go();

int main();
void _start() __attribute__((export_name("_start"))) {
	main();
}

int main() __attribute__((export_name("main"))) {
	go();
	return 0;
}
