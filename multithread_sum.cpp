#include <iostream>
#include <thread>
#include <mutex>	// mutual exclusion 상호 배제, lock과 unlock
#include <atomic>
#include <vector>
#include <chrono>
using namespace std;
using namespace chrono;

volatile int sum;

/* 1 */
void thread_func1(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		sum += 2;
	// 위 명령을 어셈블러로 보면
	// mov         ecx, dword ptr[sum(주소)]
	// add         ecx, 2
	// mov         dword ptr[sum(주소)], ecx
}
// 문제가 있는 프로그램, 왜? [Data Race]
// 이 프로그램은 싱글코어멀티쓰레드에서도 문제가 발생한다..!

/* 2 */
// 이렇게 바꿔보자, (3개의 어셈블러 -> 1개의 어셈블러)
void thread_func2(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		_asm add sum, 2;
}
// 멀티 코어에서는 마이크로 연산 단위로 실행되기 때문에
// 얘도 문제가 있다 [Data Race]

// 하지만 싱글코어에서는 항상 올바른 결과가 나옴
// 왜? context switching은 누가 언제 일으킬까..?
// 프로세스가 인터럽트 신호를 감지했을 때 운영체제가 수행함
// 어셈블러 한 줄이 실행되는 동안은 인터럽트 신호를 감지할 수 없다!

/* 3 */
// 오류를 없애기 위해 lock을 걸어보자
mutex sum_lock;		// 전역으로 선언해줘야겠지?

void thread_func3(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i) {
		sum_lock.lock();
		// Critical Section
		sum += 2;
		sum_lock.unlock();
	}
	// 제대로 된 결과가 나왔다! 하지만 엄청난 오버헤드..
}

/* 4 */
// lock을 쓰지 않고 atomic하게 실행해보자.
void thread_func4(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		_asm lock add sum, 2;

	// 이런 식의 코딩은 x86에서만 동작하고, 호환성도 떨어짐
}

/* 5 */
// 표준 라이브러리 <atomic>을 사용해보자.
atomic<int> atm_sum;

void thread_func5(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		atm_sum += 2;
		// 주의! atm_sum = atm_sum + 2;
		// 두 문장은 다르다, 어떻게 다를까?

	// 호환성도 있고, x64에서도 돌아감!
	// lock보다는 낫지만 여전히 싱글쓰레드보다 멀티쓰레드가 느리다!
}

/* 6 */
// 멀티쓰레드 프로그래밍은 
// lock과 atomic연산을 최대한 줄이는 방향으로 설계해야함

// 올바른 멀티쓰레드 프로그래밍
void optimal_thread_func(int num_thread) {
	volatile int local_sum = 0;
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		local_sum += 2;

	atm_sum += local_sum;
}


int main() 
{
	// 쓰레드를 늘려가면서 성능 비교
	for (int count = 1; count <= 16; count *= 2) {
		sum = 0;
		atm_sum = 0;
		vector<thread> threads;

		auto start_t = high_resolution_clock::now();

		for (int i = 0; i < count; ++i)
			threads.emplace_back(optimal_thread_func, count);

		for (auto& t : threads) t.join();

		auto end_t = high_resolution_clock::now();

		cout << "Number of Thread = " << count << ",  ";
		cout << duration_cast<milliseconds>(end_t - start_t).count() << "ms,  ";
		
		cout << "Sum = ";
		(sum) ? cout << sum << endl : cout << atm_sum << endl;
	}
}
