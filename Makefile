CXXFLAGS := --std=c++20

%: %.cpp helper.h
	$(CXX) $(CXXFLAGS) $< -o $@
