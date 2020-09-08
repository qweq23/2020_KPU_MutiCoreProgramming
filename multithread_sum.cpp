#include <iostream>
#include <thread>
#include <mutex>	// mutual exclusion ��ȣ ����, lock�� unlock
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
	// �� ����� ������� ����
	// mov         ecx, dword ptr[sum(�ּ�)]
	// add         ecx, 2
	// mov         dword ptr[sum(�ּ�)], ecx
}
// ������ �ִ� ���α׷�, ��? [Data Race]
// �� ���α׷��� �̱��ھ��Ƽ�����忡���� ������ �߻��Ѵ�..!

/* 2 */
// �̷��� �ٲ㺸��, (3���� ����� -> 1���� �����)
void thread_func2(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		_asm add sum, 2;
}
// ��Ƽ �ھ���� ����ũ�� ���� ������ ����Ǳ� ������
// �굵 ������ �ִ� [Data Race]

// ������ �̱��ھ���� �׻� �ùٸ� ����� ����
// ��? context switching�� ���� ���� ����ų��..?
// ���μ����� ���ͷ�Ʈ ��ȣ�� �������� �� �ü���� ������
// ����� �� ���� ����Ǵ� ������ ���ͷ�Ʈ ��ȣ�� ������ �� ����!

/* 3 */
// ������ ���ֱ� ���� lock�� �ɾ��
mutex sum_lock;		// �������� ��������߰���?

void thread_func3(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i) {
		sum_lock.lock();
		// Critical Section
		sum += 2;
		sum_lock.unlock();
	}
	// ����� �� ����� ���Դ�! ������ ��û�� �������..
}

/* 4 */
// lock�� ���� �ʰ� atomic�ϰ� �����غ���.
void thread_func4(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		_asm lock add sum, 2;

	// �̷� ���� �ڵ��� x86������ �����ϰ�, ȣȯ���� ������
}

/* 5 */
// ǥ�� ���̺귯�� <atomic>�� ����غ���.
atomic<int> atm_sum;

void thread_func5(int num_thread) {
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		atm_sum += 2;
		// ����! atm_sum = atm_sum + 2;
		// �� ������ �ٸ���, ��� �ٸ���?

	// ȣȯ���� �ְ�, x64������ ���ư�!
	// lock���ٴ� ������ ������ �̱۾����庸�� ��Ƽ�����尡 ������!
}

/* 6 */
// ��Ƽ������ ���α׷����� 
// lock�� atomic������ �ִ��� ���̴� �������� �����ؾ���

// �ùٸ� ��Ƽ������ ���α׷���
void optimal_thread_func(int num_thread) {
	volatile int local_sum = 0;
	for (int i = 0; i < 5000'0000 / num_thread; ++i)
		local_sum += 2;

	atm_sum += local_sum;
}


int main() 
{
	// �����带 �÷����鼭 ���� ��
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
