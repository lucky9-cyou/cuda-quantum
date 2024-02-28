// Compile and run with:
// ```
// nvq++ --target emulate --emulate-url=https://127.0.0.1:3000 --disable-qubit-mapping emulate.cpp -o out.x && CUDAQ_LOG_LEVEL=info ./out.x
// ```
// Assumes a valid set of credentials have been stored.
// To first confirm the correctness of the program locally,
// Add a --emulate to the `nvq++` command above.

#include <cudaq.h>
#include <fstream>

// Define a simple quantum kernel to execute on Quantinuum.
struct ghz {
  // Maximally entangled state between 5 qubits.
  auto operator()() __qpu__ {
    cudaq::qvector q(5);
    h(q[0]);
    for (int i = 0; i < 4; i++) {
      x<cudaq::ctrl>(q[i], q[i + 1]);
    }
    mz(q);
  }
};

int main() {
  // Submit to Enumate synchronously (e.g., wait for the job
  // result to be returned before proceeding).
  auto counts = cudaq::sample(ghz{});
  counts.dump();
}
