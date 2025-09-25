namespace main
{
    // structs 

    struct s
    {
        int a;
        int b;
        int c;
    }

    // default types

    const int  signed_int = -1;   // 4 bytes
    const uint unsigned_int = 1; // 4 bytes

    const char  signed_char = 'a'    // 1 byte
    const uchar unsigned_char = 'a' // 1 byte

    const float  signed_float = 1.5f;     // 4 bytes
    const ufloat unsigned_float = 1.5f;  // 4 bytes

    const double signed_double = 1.5d;    // 4 bytes
    const double unsigned_double = 1.5d;  // 4 bytes

    s data 
    {
        a = 1;
        b = 2;
        c = 3;
    };

    // custom sized types

    const int48 custom_int = 1; // 48 bits
    const ufloat15 custom_float = 15.5 // 15 bits

    int main()
    {
        // Arithmetic Operators

        int si = 10;
        uint ui = 20u;
        float sf = 1.5f;
        double sd = 2.75d;
        int48 ci = 1000;
        ufloat15 uf = 15.5;

        // Basic arithmetic
        int sum = si + ui;    // addition
        int diff = si - ui;   // subtraction
        int prod = si * ci;   // multiplication
        int quot = ui / 3;    // integer division
        int mod = ui % 7;     // modulus (integers only)

        // Floating point arithmetic
        float fsum = sf + uf;
        double dprod = sd * sf;
        double ddiv = sd / 1.1d;

        // Increment / Decrement
        si++; // post-increment
        si--; // post-decrement
        ++ui; // pre-increment
        --ui; // pre-decrement

        // Compound assignments
        si += 5;
        si -= 3;
        si *= 2;
        si /= 4;
        si %= 6;

        sf += 0.5f;
        sf *= 1.1f;

        sd -= 0.75d;
        sd /= 2.0d;

        ci += 123;    // works with custom-sized ints
        ci *= 2;

        uf += 1.5;
        uf /= 2.0;
    }
}