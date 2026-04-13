{** Test logical operators with literal integers **}
void main() {
    if (1 && 1) { println_int(1); } else { println_int(0); }
    if (1 || 0) { println_int(1); } else { println_int(0); }
    if (!0) { println_int(1); } else { println_int(0); }
}