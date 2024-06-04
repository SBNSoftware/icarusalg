#ifndef ICARUSALG_TEST_GEOMETRY_CHANNELMAPCONFIG_H
#define ICARUSALG_TEST_GEOMETRY_CHANNELMAPCONFIG_H

#include "fhiclcpp/ParameterSet.h"

namespace testing {
  template <typename Environment>
  fhicl::ParameterSet wireReadoutConfig(Environment const& environment)
  {
    auto result = environment.ServiceParameters("WireReadout")
                    .template get<fhicl::ParameterSet>("Mapper");
    result.erase("tool_type");
    return result;
  }
}

#endif
