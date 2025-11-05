
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <queue>

// --- Shared State and Thread Control ---
// Global flag to signal all threads to exit.
std::atomic<bool> is_running{true};
std::atomic<bool> initialized{false};
std::atomic<bool> animation{false};
//std::atomic<bool> run_once{true};

// config.txt inputs
#include <fstream>

std::ifstream configFile;
int num_cpu;
std::string scheduler;
int quantum_cycles;
int batch_process_freq;
int max_ins;
int min_ins;
int delays_per_exec;

// Shared state for the keyboard handler and command interpreter.
std::queue<std::string> command_queue;
std::mutex command_queue_mutex;

// The marquee logic thread and display thread share this variable.
std::string marquee_display_buffer = "CSOPESY MARQUEE          ";
std::mutex marquee_to_display_mutex;

int refresh_rate_ms = 200;
int commandCount = 0;

// --- Utility Function ---
// Moves the cursor to a specific (x, y) coordinate on the console.
void gotoxy(int x, int y) {
    std::cout << "\033[" << y << ";" << x << "H";
}

<<<<<<< Updated upstream
// --- Thread Functions ---
=======
std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");
    return ss.str();
}

// --- Instruction Generation ---
Instruction make_random_instruction(int depth) {
    Instruction inst;
    int type;

    if (depth >= 3) {
        type = rng() % 5;
    }
    else {
        type = rng() % 6;
    }
    switch (type) {
    case 0: // PRINT
        //inst.type = PRINT;
        //inst.var = "";
        //break;

    case 1: // DECLARE
        inst.type = DECLARE;
        inst.var1 = "x" + std::to_string(rng() % 5);
        inst.value2 = rng() % 500;
        break;

    case 2: // ADD
        inst.type = ADD;
        inst.var1 = "x" + std::to_string(rng() % 5);
        inst.isVar2 = (rng() % 2) == 0;
        inst.isVar3 = (rng() % 2) == 0;
        if (inst.isVar2) {
            inst.var2 = "x" + std::to_string(rng() % 5);
        }
        else {
            inst.value2 = rng() % 500;
        }
        if (inst.isVar3) {
            inst.var3 = "x" + std::to_string(rng() % 5);
        }
        else {
            inst.value3 = rng() % 500;
        }
        break;

    case 3: // SUBTRACT
        inst.type = SUBTRACT;
        inst.var1 = "x" + std::to_string(rng() % 5);
        inst.isVar2 = (rng() % 2) == 0;
        inst.isVar3 = (rng() % 2) == 0;
        if (inst.isVar2) {
            inst.var2 = "x" + std::to_string(rng() % 5);
        }
        else {
            inst.value2 = rng() % 500;
        }
        if (inst.isVar3) {
            inst.var3 = "x" + std::to_string(rng() % 5);
        }
        else {
            inst.value3 = rng() % 500;
        }
        break;

    case 4: // SLEEP
        inst.type = SLEEP;
        inst.sleepTicks = (rng() % 5) + 1;
        break;

    case 5: // FOR_LOOP
        inst.type = FOR_LOOP;
        inst.repeatCount = (rng() % 3) + 2;
        int nested_count = (rng() % 3) + 1;
        for (int i = 0; i < nested_count; i++) {
            inst.nestedInstructions.push_back(make_random_instruction(depth + 1));
        }
        break;
    }

    return inst;
}

PCB* create_random_process(int pid) {
    PCB* p = new PCB();
    p->pid = pid;
    p->name = "process_" + std::to_string(pid);
    p->start_time = std::chrono::system_clock::now();

    int num_instructions = min_ins + (rng() % (max_ins - min_ins + 1));
    p->total_instructions = num_instructions;

    for (int i = 0; i < num_instructions; i++) {
        p->instructions.push_back(make_random_instruction(0));
    }

    return p;
}

PCB* create_named_process(const std::string& name, int pid) {
    PCB* p = create_random_process(pid);
    p->name = name;
    return p;
}

// --- Instruction Execution ---
void execute_instruction(PCB& p, Instruction& inst) {
    switch (inst.type) {
    case PRINT: {
        std::string output;
        if (!inst.var.empty()) {
            output = "Hello world from " + p.name + "! Value: " + std::to_string(p.vars[inst.var]);
        }
        else {
            output = "Hello world from " + p.name + "!";
        }
        p.screenBuffer.push_back("(" + get_timestamp() + ") " + output);
        break;
    }
    case DECLARE:
        p.vars[inst.var1] = inst.value2;
        break;

    case ADD: {
        uint16_t a = inst.isVar2 ? p.vars[inst.var2] : inst.value2;
        uint16_t b = inst.isVar3 ? p.vars[inst.var3] : inst.value3;
        p.vars[inst.var1] = a + b;
        break;
    }

    case SUBTRACT: {
        uint16_t a = inst.isVar2 ? p.vars[inst.var2] : inst.value2;
        uint16_t b = inst.isVar3 ? p.vars[inst.var3] : inst.value3;
        p.vars[inst.var1] = a - b;
        break;
    }

    case SLEEP:
        p.sleep_ticks = inst.sleepTicks;
        return;

    case FOR_LOOP: {
        for (int i = 0; i < inst.repeatCount; i++) {
            for (auto& nested : inst.nestedInstructions) {
                execute_instruction(p, nested); 
                if (p.finished) return;
            }
        }
        break;
    }
    }

    p.pc++;
    if (p.pc >= (int)p.instructions.size()) {
        p.finished = true;
        p.end_time = std::chrono::system_clock::now();
    }
}

// --- CPU Worker ---
void cpu_worker(int id) {
    PCB* current_process = nullptr;
    int current_run_cycles = 0; // tracks cycles for the current quantum

    while (scheduler_running) {
        if (scheduler == "rr") {
            // --- 1. Load a New Process if CPU is Idle or Previous was Preempted/Finished ---
            if (current_process == nullptr) {
                std::unique_lock<std::mutex> lock(readyQueueMutex);
                if (!readyQueue.empty()) {
                    current_process = readyQueue.front();
                    readyQueue.pop();
                    current_run_cycles = 0;
                }
            }

            // --- 2. Execute or Handle Current Process ---
            if (current_process) {
                {
                    std::lock_guard<std::mutex> lock(cpu_stats_mutex);
                    cpu_busy[id] = true;
                    current_process->cpu_core = id;
                }

                // Handle Sleep (I/O)
                if (current_process->sleep_ticks > 0) {
                    current_process->sleep_ticks--;

                    if (current_process->sleep_ticks == 0) {
                        std::lock_guard<std::mutex> lock(readyQueueMutex);
                        readyQueue.push(current_process);
                    }
                    current_process = nullptr;
                }
                // Execute the instruction
                else {
                    execute_instruction(*current_process, current_process->instructions[current_process->pc]);
                    current_run_cycles++;

                    // Check for Termination
                    if (current_process->finished) {
                        std::lock_guard<std::mutex> lock(cpu_stats_mutex);
                        cpu_process_count[id]++;
                        current_process = nullptr;
                    }
                    // Preemption
                    else if (current_run_cycles >= quantum_cycles) {
                        std::lock_guard<std::mutex> lock(readyQueueMutex);
                        readyQueue.push(current_process);

                        current_process = nullptr; 
                    }
                }
            }
            else {
                std::lock_guard<std::mutex> lock(cpu_stats_mutex);
                cpu_busy[id] = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
    }
    // moved cleanup from main to here
    if (current_process != nullptr) {
        delete current_process;
    }
}

// --- Screen Commands ---
void display_process_screen(const std::string& process_name) {
    std::lock_guard<std::mutex> lock(process_map_mutex);

    if (all_processes.find(process_name) == all_processes.end()) {
        std::cout << "Process '" << process_name << "' not found.\n";
        return;
    }

    PCB* p = all_processes[process_name];
    clear_screen();

    std::cout << "Process: " << p->name << "\n";
    std::cout << "ID: " << p->pid << "\n";

    if (p->finished) {
        std::cout << "\nFinished!\n\n";
    }
    else {
        std::cout << "Current instruction line: " << p->pc << "\n";
        std::cout << "Lines of code: " << p->total_instructions << "\n";
    }

    std::cout << "\n--- Logs ---\n";
    int start = std::max(0, (int)p->screenBuffer.size() - 20);
    for (int i = start; i < (int)p->screenBuffer.size(); i++) {
        std::cout << p->screenBuffer[i] << "\n";
    }

    if (p->finished) {
        std::cout << "\nFinished!\n";
    }
    std::cout << "\n";
}

void process_smi() {
    if (current_process_name.empty()) {
        std::cout << "Not in a process screen.\n";
        return;
    }
    display_process_screen(current_process_name);
}

void screen_ls() {
    std::lock_guard<std::mutex> plock(process_map_mutex);
    std::lock_guard<std::mutex> clock(cpu_stats_mutex);

    int cores_used = 0;
    for (int i = 0; i < num_cpu; i++) {
        if (cpu_busy[i]) cores_used++;
    }

    int running = 0, finished = 0, total_processes = 0;
    for (const auto& pair : all_processes) {

        if (pair.second->finished) finished++;
        else running++;
    }

    std::cout << "\nCPU Utilization: " << (cores_used * 100 / num_cpu) << "%\n";
    std::cout << "Cores used: " << cores_used << "\n";
    std::cout << "Cores available: " << (num_cpu - cores_used) << "\n";
    std::cout << "\nTotal processes: " << total_processes << "\n";
    std::cout << "Running processes: " << running << "\n";
    std::cout << "Finished processes: " << finished << "\n";
    std::cout  << "+---------------+--------------------------+----------+-----------------------------------+" << std::endl;

    for (const auto& pair : all_processes) {
        PCB* p = pair.second;

        auto time = std::chrono::system_clock::to_time_t(p->start_time);
        std::tm timeinfo;
#ifdef _WIN32
        localtime_s(&timeinfo, &time);
#else
        localtime_r(&time, &timeinfo);
#endif

        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");
        std::cout << "| " << std::left << std::setw(14) << p->name << "| ";
        std::cout << " (" << ss.str() << ") "<< "| ";
        if (p->finished) {
            std::cout << std::right << std::setw(7) <<  "Done" << "| ";
        }
        else {
            std::cout << std::right << std::setw(7) << "Core: " << p->cpu_core << " | ";
            //std::cout << p->cpu_core << "    ";
        }
        int barWidth = 20;
        int filled = (p->pc * barWidth) / p->total_instructions;
        
       std::cout << "[";
        for (int i = 0; i < barWidth; i++) {
            if (i < filled) std::cout << "=";
            else std::cout << " ";
        }
        std::cout << "] " << std::right << std::setw(3) << p->pc << " / " << p->total_instructions << " |\n";
    }
    std::cout  << "+---------------+--------------------------+----------+-----------------------------------+" << std::endl;
}

void report_util() {
    std::ofstream report("csopesy-log.txt");
    if (!report.is_open()) {
        std::cout << "Error: Could not create report file.\n";
        return;
    }

    std::lock_guard<std::mutex> plock(process_map_mutex);
    std::lock_guard<std::mutex> clock(cpu_stats_mutex);

    int cores_used = 0;
    for (int i = 0; i < num_cpu; i++) {
        if (cpu_busy[i]) cores_used++;
    }

    int running = 0, finished = 0;
    std::vector<PCB*> running_processes, finished_processes;

    for (const auto& pair : all_processes) {
        if (pair.second->finished) {
            finished++;
            finished_processes.push_back(pair.second);
        }
        else {
            running++;
            running_processes.push_back(pair.second);
        }
    }

    report << "CPU Utilization Report\n";
    report << "Generated: " << get_timestamp() << "\n\n";
    report << "CPU Utilization: " << (static_cast<double>(cores_used) * 100.0 / num_cpu) << "%\n";
    report << "Cores used: " << cores_used << "\n";
    report << "Cores available: " << (num_cpu - cores_used) << "\n";
    report << "Running processes: " << running << "\n";
    report << "Finished processes: " << finished << "\n\n";

    report << "--------------------------------------\n";

    report << "Running processes:\n";
    for (PCB* p : running_processes) {
        auto time = std::chrono::system_clock::to_time_t(p->start_time);
        std::tm timeinfo;
#ifdef _WIN32
        localtime_s(&timeinfo, &time);
#else
        localtime_r(&time, &timeinfo);
#endif

        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");

        report << p->name << "    (" << ss.str() << ")    Core: "
            << p->cpu_core << "    " << p->pc << " / " << p->total_instructions << "\n";
    }

    report << "\nFinished processes:\n";
    for (PCB* p : finished_processes) {
        auto time = std::chrono::system_clock::to_time_t(p->start_time);
        std::tm timeinfo;
#ifdef _WIN32
        localtime_s(&timeinfo, &time);
#else
        localtime_r(&time, &timeinfo);
#endif

        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p");

        report << p->name << "    (" << ss.str() << ")    Finished    "
            << p->pc << " / " << p->total_instructions << "\n";
    }

    report << "--------------------------------------\n";

    report.close();

    std::cout << "Report generated: csopesy-log.txt\n";
}

// --- Keyboard Handler ---
>>>>>>> Stashed changes
void keyboard_handler_thread_func() {
    std::string command_line;
    while (is_running) {
        std::cout << "Command >> ";
        std::getline(std::cin, command_line);
        
        if (!command_line.empty()) {
            if (command_line == "exit")
                is_running = false;

            std::unique_lock<std::mutex> lock(command_queue_mutex);
            command_queue.push(command_line);
            commandCount++;
            lock.unlock();
        }
    }
}

// --- Main Function (Command Interpreter Thread) ---
int main() {
    // Start the three worker threads.
    std::thread keyboard_handler_thread(keyboard_handler_thread_func);

    std::cout << "\033[2J\033[1;1H"; 

    std::cout << marquee_display_buffer;
    std::cout << "\nGroup Developers:\n";
    std::cout << "1. Matthew Copon\n";
    std::cout << "2. Chastine Cabatay\n";
    std::cout << "3. Ericson Tan\n";
    std::cout << "4. Joaquin Cardino\n";
    std::cout << "Version: 1.00.00\n\n\n\n\n\n\n\n\n";
    
    // Main loop that processes commands from the queue.
    while (is_running) {
        std::string command_line;
        {
            std::unique_lock<std::mutex> lock(command_queue_mutex);
            if (!command_queue.empty()) {
                command_line = command_queue.front();
                command_queue.pop();
            }
        }
        
        if (!command_line.empty()) {
            // Command processing logic goes here...
            if (command_line == "initialize") {
                configFile.open("config.txt");
                
                if (!configFile.is_open()) {
                    std::cerr << "ERROR: config.txt could not be opened." << std::endl;
                }
                else
                {
                    std::string key, value;

                while (configFile >> key >> value) {

                    if (key == "num-cpu")
                        num_cpu = std::stoi(value);

                    else if (key == "scheduler")
                        scheduler = value.substr(1, value.size() - 2); // remove quotes

                    else if (key == "quantum-cycles")
                        quantum_cycles = std::stoi(value);

                    else if (key == "batch-processes-freq")
                        batch_process_freq = std::stoi(value);

                    else if (key == "min-ins")
                        min_ins = std::stoi(value);

                    else if (key == "max-ins")
                        max_ins = std::stoi(value);

                    else if (key == "delay-per-exec")
                        delays_per_exec = std::stoi(value);
                }

                initialized = true;
                std::cout << "Console initialized successfully.\n";
                }
            }
            // Every  X CPU ticks, a new process is randomly generated and put into the 
            // ready queue for  your  CPU  scheduler. This frequency can be set in the “config.txt.”
            // command can only be accessible in the main menu console
            else if (command_line == "scheduler_start") {
                if (initialized){
                    
                }
                else
                    std::cout << "ERROR: Console not initialized. Please initialize with 'initialize' command." << std::endl;
                
            }
            // stops generating dummy processes
            // command can only be accessible in the main menu console
            else if (command_line == "scheduler_stop") {
                if (initialized){

                }
                else
                    std::cout << "ERROR: Console not initialized. Please initialize with 'initialize' command." << std::endl;
            }
            // for generating CPU utilization report.
            // all finished and currently running processes
            // same info with screen -ls, only difference is that report-util saves this into a text file
            else if (command_line == "report_util") {
                if (initialized){

                }
                else
                    std::cout << "ERROR: Console not initialized. Please initialize with 'initialize' command." << std::endl;
            }
            // creates a new process via "screen -s <process name>" command
            // screen -s <process name> -- console will clear its content and move to the process screen
            // from there, the user can type
                // "process-smi" - prints a simple info about the process, provides updated details and accompanying logs
                // if a process has finished, simply print "Finished!" after the processname, ID, and logs
            // "screen-ls" -- list CPU utilization, cores used, and cores available, summary of running and finished processes
            // "screen -r <process name>" - user can access the screen any time through this
            // if process name is not found / finished execution, prints "Process <process name> not fount"
            else if (command_line.rfind("screen", 0) == 0) {
                if (initialized){

                }
                else
                    std::cout << "ERROR: Console not initialized. Please initialize with 'initialize' command." << std::endl;
            }
            else {
                gotoxy(12, 15 + commandCount);
                std::cout << "Command not found.";
                gotoxy(12, 16 + commandCount);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Join threads to ensure they finish cleanly.
    if (keyboard_handler_thread.joinable()) {
        keyboard_handler_thread.join();
    }

    return 0;
}
