{** 结构体与 switch 语句：星期计算 **}

struct Date {
    int32 day;
    int32 month;
    int32 year;
}

int32 day_of_week(int32 day) {
    int32 result;
    switch (day) {
        case 1: result = 1; break;
        case 2: result = 2; break;
        case 3: result = 3; break;
        case 4: result = 4; break;
        case 5: result = 5; break;
        case 6: result = 6; break;
        case 7: result = 7; break;
        default: result = 0; break;
    }
    return result;
}

void main() {
    struct Date d;
    d.day = 15;
    d.month = 4;
    d.year = 2026;
    int32 dow;
    dow = day_of_week(d.day);
    print_string("Day of week: ");
    println_int(dow);
}