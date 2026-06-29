
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <limits>

using namespace std;

struct Customer {
    int    id;
    double iat;            // inter-arrival time
    double arrivalTime;    // clock time of arrival
    double serviceTime;    // service duration
    double serviceStart;   // time service begins
    double serviceEnd;     // time service ends (departure)
    double waitTime;       // time spent waiting in queue
    double timeInSystem;   // wait + service
};

// ---------- Utility: clear bad input ----------
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// ---------- INPUT WINDOW ----------
struct SimParams {
    int    numCustomers = 100;
    double iatLow = 1.0,  iatHigh = 8.0;   // arrival uniform range
    double stLow  = 1.0,  stHigh  = 6.0;   // service uniform range
};

SimParams inputWindow() {
    SimParams p;
    cout << "==================================================\n";
    cout << "            INPUT WINDOW  -  Simulation Setup       \n";
    cout << "==================================================\n";

    cout << "Number of customers to simulate [default 100]: ";
    string line;
    getline(cin, line);
    if (!line.empty()) {
        try { p.numCustomers = stoi(line); } catch (...) { p.numCustomers = 100; }
    }

    cout << "\nSetup confirmed:\n";
    cout << "  Customers      : " << p.numCustomers << "\n";
    cout << "  IAT range      : (" << p.iatLow << ", " << p.iatHigh << ") minutes\n";
    cout << "  Service range  : (" << p.stLow  << ", " << p.stHigh  << ") minutes\n\n";

    return p;
}

// ---------- SIMULATION ENGINE ----------
vector<Customer> runSimulation(const SimParams& p) {
    vector<Customer> customers(p.numCustomers);

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> iatDist(p.iatLow, p.iatHigh);
    uniform_real_distribution<double> stDist(p.stLow, p.stHigh);

    double clock = 0.0;
    double serverFreeAt = 0.0;

    for (int i = 0; i < p.numCustomers; ++i) {
        Customer& c = customers[i];
        c.id = i + 1;
        c.iat = iatDist(gen);
        clock += c.iat;
        c.arrivalTime = clock;
        c.serviceTime = stDist(gen);

        c.serviceStart = max(c.arrivalTime, serverFreeAt);
        c.waitTime = c.serviceStart - c.arrivalTime;
        c.serviceEnd = c.serviceStart + c.serviceTime;
        c.timeInSystem = c.waitTime + c.serviceTime;

        serverFreeAt = c.serviceEnd;
    }
    return customers;
}

// ---------- OUTPUT WINDOW ----------
void outputWindow(const vector<Customer>& customers) {
    cout << "==================================================\n";
    cout << "           OUTPUT WINDOW  -  Simulation Result       \n";
    cout << "==================================================\n\n";

    cout << left
         << setw(6)  << "Cust"
         << setw(10) << "IAT"
         << setw(12) << "ArrTime"
         << setw(10) << "SvcTime"
         << setw(12) << "SvcStart"
         << setw(12) << "SvcEnd"
         << setw(10) << "Wait"
         << setw(12) << "InSystem"
         << "\n";
    cout << string(94, '-') << "\n";

    double totalWait = 0, totalService = 0, totalInSystem = 0;
    double idleTime = 0;
    double lastEnd = 0;
    int    numWaited = 0;

    cout << fixed << setprecision(2);
    for (size_t i = 0; i < customers.size(); ++i) {
        const Customer& c = customers[i];
        cout << left
             << setw(6)  << c.id
             << setw(10) << c.iat
             << setw(12) << c.arrivalTime
             << setw(10) << c.serviceTime
             << setw(12) << c.serviceStart
             << setw(12) << c.serviceEnd
             << setw(10) << c.waitTime
             << setw(12) << c.timeInSystem
             << "\n";

        totalWait     += c.waitTime;
        totalService  += c.serviceTime;
        totalInSystem += c.timeInSystem;
        if (c.waitTime > 0.0001) numWaited++;

        double gap = c.serviceStart - lastEnd;
        if (gap > 0) idleTime += gap;
        lastEnd = c.serviceEnd;

        // Pause every 25 rows so long tables are readable interactively
        if ((i + 1) % 25 == 0 && i + 1 != customers.size()) {
            cout << "\n-- press ENTER to continue --";
            cin.get();
        }
    }

    int n = (int)customers.size();
    double totalTime = lastEnd; // total simulation clock time
    double avgWait        = totalWait / n;
    double avgService     = totalService / n;
    double avgInSystem    = totalInSystem / n;
    double probWait       = (double)numWaited / n * 100.0;
    double serverUtil     = (totalTime > 0) ? (totalService / totalTime) * 100.0 : 0;
    double idlePercent    = 100.0 - serverUtil;

    cout << "\n==================================================\n";
    cout << "                 QUEUE STATISTICS                  \n";
    cout << "==================================================\n";
    cout << "Total customers served        : " << n << "\n";
    cout << "Total simulation time         : " << totalTime   << " minutes\n";
    cout << "Average inter-arrival time    : " << (totalTime/n) << " minutes\n";
    cout << "Average service time          : " << avgService << " minutes\n";
    cout << "Average waiting time in queue : " << avgWait     << " minutes\n";
    cout << "Average time in system        : " << avgInSystem << " minutes\n";
    cout << "Number of customers who waited: " << numWaited << " (" << probWait << "%)\n";
    cout << "Server idle time              : " << idleTime << " minutes\n";
    cout << "Server utilization            : " << serverUtil  << " %\n";
    cout << "Server idle percentage        : " << idlePercent << " %\n";
    cout << "==================================================\n";
}

// ---------- MAIN: keeps input & output windows interactive ----------
int main() {
    char again = 'y';
    while (again == 'y' || again == 'Y') {
        SimParams params = inputWindow();
        vector<Customer> customers = runSimulation(params);
        outputWindow(customers);

        cout << "\nRun another simulation? (y/n): ";
        cin >> again;
        clearInput();
        cout << "\n";
    }
    cout << "Simulation ended. Goodbye!\n";
    return 0;
}