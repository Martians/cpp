
namespace cpp test

struct WorkTask {
    1: i32 V32
    2: i64 V64
    3: string Vstr 
}

service WorkService {
    
    i32 ping(1:i32 value)
    
    WorkTask trigger(1:i64 seqno, 2:WorkTask task)
}
