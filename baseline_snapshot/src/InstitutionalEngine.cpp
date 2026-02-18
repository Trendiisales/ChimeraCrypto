#include "InstitutionalEngine.hpp"

namespace chimera {

InstitutionalEngine::InstitutionalEngine(double initial_equity)
    : equity_(initial_equity)
{}

void InstitutionalEngine::tick(size_t)
{
    equity_ += 1.0;
}

void InstitutionalEngine::update_book(const std::string&,
                                      const std::vector<double>&,
                                      const std::vector<double>&)
{
}

void InstitutionalEngine::add_trade(const std::string&,
                                    double,
                                    double)
{
}

double InstitutionalEngine::get_equity() const
{
    return equity_;
}

}
