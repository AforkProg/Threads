#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <iostream>
using namespace std;
class ThreadPool
{
public:
	using task = function<void()>;
	ThreadPool(size_t threadsNum)
	{
		start(threadsNum);
	}
	template<class T>
	auto enqueue(T task)->future<decltype(task())>
	{
		auto wrap = make_shared<packaged_task<decltype(task())()>>(move(task));
		{
			unique_lock<mutex> lock(eventMtx);
			tasks.emplace([=] {
				(*wrap)();
			});
		}
		eventCv.notify_one();
		return wrap->get_future();
	}
	~ThreadPool()
	{
		stop();
	}
private:
	vector<thread> threads;
	condition_variable eventCv;
	mutex eventMtx;
	bool stopFlag = false;
	queue<task> tasks;
	void start(size_t threadsMum)
	{
		for (int a = 0; a < threadsMum; a++)
		{
			threads.emplace_back([=] {
				while (true)
				{
					task task;
					{
						unique_lock<mutex> lock(eventMtx);
						eventCv.wait(lock, [=] { return stopFlag || !tasks.empty(); });
						if (stopFlag && tasks.empty())
							break;
						task = move(tasks.front());
						tasks.pop();
					}
					task();
				}
			});
		}
	}
	void stop()
	{
		unique_lock<mutex> lock(eventMtx);
		stopFlag = true;

		eventCv.notify_all();
		for (auto &thread : threads)
		{
			thread.join();
		}
	}
};
void test()
{
	cout << "lolz" << endl;
}
void test1()
{
	cout << "kek" << endl;
}
int main()
{
	ThreadPool pool{ 10 };
	pool.enqueue([] {
		test();
		test1();
	});
	auto first = pool.enqueue([] {
		return "chop";
	});
	auto second = pool.enqueue([] {
		return 3;
	});
	cout << (first.get()) << "   " << (second.get()) << endl;
	system("pause");
	return 0;
}