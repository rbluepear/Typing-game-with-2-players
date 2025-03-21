#include "../thirdparty/httplib.h"
#include <mutex>
#include <string>
#include <fstream>

// Storing the text in server can avoid client modifies the target by chance
std::string str_text; // The task this game uses

std::mutex g_mutex; // Avoid the global var g_str be modified by multi-threads

int progress_1 = -1; // Player1's progress, init = -1, start = 0
int progress_2 = -1; // Player2's progress

void loding_ok(const httplib::Request& req, httplib::Response& res) {
	std::lock_guard<std::mutex> lock(g_mutex); // Lock, be destroyed when leaves this woid func

	if (progress_1 >= 0 && progress_2 >= 0) { // Iff 2 players in game
		res.set_content("-1", "text/plain");
		return;
	}

	res.set_content(progress_1 >= 0 ? "2" : "1", "text/plain"); // 分配id to player
	(progress_1 >= 0) ? (progress_2 = 0) : (progress_1 = 0);

	// lock will be destroyed after the line, no need to unlock
}

int main(int argc, char** argv) {

	/* Open the text file. */
	std::ifstream file("text.txt");
	if (!file.good()) {
		MessageBox(nullptr, L"无法打开文本文件 text.txt", L"启动失败", MB_OK | MB_ICONERROR);
		return -1;
	}

	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_text = str_stream.str();

	file.close();

	/* Server */
	httplib::Server server;

	// 建立连接
	/*
	server.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
		std::lock_guard<std::mutex> lock(g_mutex); // Lock, be destroyed when leaves this woid func

		if (progress_1 >= 0 && progress_2 >= 0) { // Iff 2 players in game
			res.set_content("-1", "text/plain");
			return;
		}

		res.set_content(progress_1 >= 0 ? "2" : "1", "text/plain"); // 分配id to player
		(progress_1 >= 0) ? (progress_2 = 0) : (progress_1 = 0);
		});
	*/
	server.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
		std::lock_guard<std::mutex> lock(g_mutex);

		if (progress_1 >= 0 && progress_2 >= 0) {
			res.set_content("-1", "text/plain");
			return;
		}

		// 分配 ID 并设置进度
		if (progress_1 < 0) {
			res.set_content("1", "text/plain");
			progress_1 = 0; // 玩家1加入，设置 progress_1 = 0
		}
		else {
			res.set_content("2", "text/plain");
			progress_2 = 0; // 玩家2加入，设置 progress_2 = 0
		}
		});

	// 传输任务文本
	server.Post("/query_text", [&](const httplib::Request& req, httplib::Response& res) {
		res.set_content(str_text, "text/plain"); // Send the target to clients
		});
	// 
	server.Post("/update_1", [&](const httplib::Request& req, httplib::Response& res) {
		std::lock_guard<std::mutex> lock(g_mutex);
		progress_1 = std::stoi(req.body); // Get the progress of player1
		res.set_content(std::to_string(progress_2), "text/plain"); // tell player2 the progress of 1
		});
	server.Post("/update_2", [&](const httplib::Request& req, httplib::Response& res) {
		std::lock_guard<std::mutex> lock(g_mutex);
		progress_2 = std::stoi(req.body);
		res.set_content(std::to_string(progress_1), "text/plain");
		});
	server.listen("localhost", 25565);
	return 0;
}