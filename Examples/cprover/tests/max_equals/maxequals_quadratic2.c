#define max(A,B) ( (A > B) ? A : B )

int x;
int n;

void loop() {
    for(int t = 0; t <= 10; t++) {
        x = max(x, 10 * t - t * t);
    }
}

void main(int initial) {
    x = initial;
    loop();
    __VERIFIER_print_hull(x);
    __VERIFIER_assert(x >= initial);
}
