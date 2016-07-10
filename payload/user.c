const char *needle = "\001\001SceWebBrowser";

int memcmp(const void* s1, const void* s2,unsigned n)
{
	const unsigned char *p1 = s1, *p2 = s2;
	while(n--)
		if( *p1 != *p2 )
			return *p1 - *p2;
		else
			p1++,p2++;
	return 0;
}

void __attribute__ ((section (".text.start"))) user_payload(int args, void *argp) {
	unsigned pc = ((unsigned*)argp)[0];
	while (pc--) {
		if (memcmp(pc, needle, 15) == 0) {
			break;
		}
	}
	unsigned browser_base = pc - 0xc5856;
	void (*debug_print)() = browser_base + 0xC2A44;
	debug_print("hello from the browser!\n");
	while (1) {}
}
