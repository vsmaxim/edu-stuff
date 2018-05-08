#include <stdio.h>

struct ass {
	int a;
	int b;
};

int main()
{
	ass* a = (ass*)malloc(sizeof(ass));
	a->a = 123;
	printf("%d", a->a);
	return 0;
}
