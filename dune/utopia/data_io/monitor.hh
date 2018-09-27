#ifndef UTOPIA_DATAIO_MONITOR_HH
#define UTOPIA_DATAIO_MONITOR_HH

#include <vector>
#include <chrono>
#include <iostream>

#include <yaml-cpp/yaml.h>

namespace Utopia{
namespace DataIO{

template <typename DurationType = std::chrono::seconds>
class MonitorTimer{
public:
    typedef std::chrono::high_resolution_clock Clock;

    using Time = std::__1::chrono::high_resolution_clock::time_point;

private:
    const double _emit_interval;

    Time _last_emit;

public:
    MonitorTimer(const double emit_interval) :
                    _emit_interval(emit_interval),
                    // Initialize the time of the last commit to current time
                    _last_emit(Clock::now()) {}

    bool time_has_come(){
        // Calculate the time difference between now and the last emit
        auto now = Clock::now();
        auto duration = std::chrono::duration_cast<DurationType>(now - _last_emit);
        
        // If more time than the _emit_interval has passed return true
        if (duration.count() > _emit_interval) {
            _last_emit = now;
            return true;
        }
        else{
            return false;
        }
    }
};


class MonitorData{
private:
    YAML::Node _data;
public:
    MonitorData() : _data(YAML::Node()) {
        // Specify that the emitted data is shown in a single line
        _data.SetStyle(YAML::EmitterStyle::Flow);
    };

    // YAML::Node& get_data(){
    //     return _data;
    // }

    template<typename Value>
    void set_entry(const std::string model_name, const std::string key, const Value value){
        _data[model_name + "." + key] = value;
    }

    void emit(){
        std::cout << _data << std::endl;
    }
};


class Monitor{
private:
    const std::string _name;

    std::shared_ptr<MonitorTimer<>> _timer;

    std::shared_ptr<MonitorData> _data;
public:
    Monitor(const std::string name,
            std::shared_ptr<MonitorTimer<>>& timer,
            std::shared_ptr<MonitorData>& data) : 
                _name(name),
                _timer(timer),
                _data(data) {}

    template <typename Value>
    void set_entry(const std::string key, const Value value){
        // Set the data
        _data->set_entry(_name, key, value);
    }

    template <typename Function>
    void set_entry(const std::string key, const Function& function){
        _data->set_entry(_name, key, function());
    }

    template <typename Value>
    void set_entry_if_time_is_ripe(const std::string key, const Value value){
        if (_timer->time_has_come()){
            set_entry(key, value);   
        }
    }

    template <typename Function>
    void set_entry_if_time_is_ripe(const std::string key, Function& function){
        if (_timer->time_has_come()){
            set_entry(key, function);   
        }
    }
};


class RootMonitor{
private:
    const std::string _name;

    std::shared_ptr<MonitorTimer<>> _timer;

    MonitorData _data_to_emit;

public:
    RootMonitor(const std::string name, 
                const double emit_interval) :
                    _name(name),
                    _timer(std::make_shared<MonitorTimer<>>(emit_interval)),
                    // Create an empty MonitorData object for the data to be emitted
                    _data_to_emit(MonitorData()) {};

    void perform_emission(){
        _data_to_emit.emit();
    }

    bool time_has_come(){
        return _timer->time_has_come();
    }

    // YAML::Node& get_data_to_emit(){
    //     return _data_to_emit.get_data();
    // }

    std::shared_ptr<MonitorTimer<>> get_timer(){
        return _timer;
    }
};



} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_MONITOR_HH