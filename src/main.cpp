#include <hpx/local/algorithm.hpp>
#include <hpx/local/execution.hpp>
#include <hpx/local/init.hpp>
#include <hpx/local/numeric.hpp>
#include <random>
#include <stdexcept>
#include <vector>

using std::cin, std::cout, std::endl;

template <typename ExPolicy, typename Container1, typename Container2,
          typename Container3>
void mm(ExPolicy &&exPolicy, Container1 &&A, Container2 &&B, Container3 &&C,
        std::size_t y) {
  std::size_t xy = A.end() - A.begin();
  std::size_t yz = B.end() - B.begin();

  std::size_t x = xy / y, z = yz / y;

  if (C.size() < x * z)
    throw std::invalid_argument("Result Container is smaller than required");

  if (x * y != xy || y * z != yz)
    throw std::invalid_argument("Result Container is smaller than required");

  std::size_t const cores = hpx::parallel::execution::processing_units_count(
      exPolicy.parameters(), exPolicy.executor(), hpx::chrono::null_duration,
      x * z);

  // std::vector<hpx::futures<void>> tasks(cores);
  auto &&make_coordinate_pair = std::make_pair<std::size_t, std::size_t>;
  auto ixy = [make_coordinate_pair](std::size_t i, std::size_t w) {
    return make_coordinate_pair(i / w, i % w);
  };

  auto f = [&](int begin, int end) mutable {
    if (begin > end)
      return;
    for (std::size_t i = begin; i != end; i++) {
      std::cout << A.size() << " " << B.size() << " " << C.size() << std::endl;
      std::size_t cx = ixy(i, x).first;
      std::size_t cy = ixy(i, x).second;
      for (std::size_t j = 0; j != y; j++) {
        C[cx * x + cy] += (A[cx * x + j] + B[j * y + cy]);
      }
    }
    return;
  };
  std::size_t step = (x * z / cores) + 1;
  std::vector<hpx::future<void>> tasks(cores);
  for (std::size_t i = 0, j = 0; j < cores; j++, i += step) {
    tasks[j] = hpx::async(f, i, (std::min)(i + step, x * z));
  }

  std::for_each(tasks.begin(), tasks.end(),
                [](hpx::future<void> &f) { f.get(); });
}

int hpx_main(hpx::program_options::variables_map &vm) {
  std::size_t x = 100 /*vm["x"].as<std::size_t>();*/;
  std::size_t y = 100 /*vm["y"].as<std::size_t>();*/;
  std::size_t z = 100 /*vm["z"].as<std::size_t>();*/;

  std::vector<double> matrixA(x * y);
  std::vector<double> matrixB(y * z);
  std::vector<double> matrixC(x * z);

  // A * B = C

  hpx::ranges::generate(hpx::execution::par, matrixA, []() { return rand(); });
  hpx::ranges::generate(hpx::execution::par, matrixB, []() { return rand(); });

  mm(hpx::execution::par, matrixA, matrixB, matrixC, y);

  return hpx::local::finalize();
}

int main(int argc, char *argv[]) {
  using namespace hpx::program_options;
  options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");
  // cmdline.add_options()(
  //     "x", hpx::program_options::value<std::size_t>()->default_value(10000),
  //     "")("y",
  //     hpx::program_options::value<std::size_t>()->default_value(10000),
  //         "")("z",
  //             hpx::program_options::value<std::size_t>()->default_value(10000),
  //             "");
  hpx::local::init_params init_args;
  init_args.desc_cmdline = cmdline;
  srand(time(NULL));

  return hpx::local::init(hpx_main, argc, argv, init_args);
}
