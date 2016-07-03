#ifndef LOGGER_H
#define LOGGER_H

/*
 * adapted from http://www.drdobbs.com/cpp/a-lightweight-logger-for-c/240147505
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

#include <ctime>

enum severity_type
{
    debug,
    error,
    warning
};

class log_policy_interface
{
public:
    virtual ~log_policy_interface()
    {
    }

    virtual void open_ostream(const std::string &name) = 0;
    virtual void close_ostream() = 0;
    virtual void write(const std::string &str) = 0;
};

class std_log_policy : public log_policy_interface
{
public:
    void open_ostream(const std::string &name) override
    {
        static_cast<void>(name);
    }

    void close_ostream() override
    {
    }

    void write(const std::string &str) override
    {
        std::clog << str << std::endl;
    }
};

class file_log_policy : public log_policy_interface
{
public:
    file_log_policy()
        : output_stream(new std::ofstream)
    {
    }

    void open_ostream(const std::string &name) override
    {
        output_stream->open(name.c_str(), std::ios_base::binary | std::ios_base::out);
        if (!output_stream->is_open())
        {
            throw std::runtime_error("logger: unable to open an output stream");
        }
    }

    void close_ostream() override
    {
        if (output_stream != nullptr)
        {
            output_stream->close();
        }
    }

    void write(const std::string &str) override
    {
        (*output_stream) << str << std::endl;
    }

    ~file_log_policy()
    {
        if (output_stream != nullptr)
        {
            close_ostream();
        }
    }

private:
    std::unique_ptr<std::ofstream> output_stream;
};

template <typename Policy>
class logger
{
public:
    logger()
        : logger(std::string())
    {
    }

    logger(const std::string &name)
    {
        policy = new Policy;

        if (policy == nullptr)
        {
            throw std::runtime_error("logger: unable to create the logger instance");
        }

        policy->open_ostream(name);
    }

    template <severity_type severity, typename ...Args>
    void print(Args ...args)
    {
        std::lock_guard<std::mutex> lock(write_lock);
        switch (severity)
        {
            case severity_type::debug:
                log_stream << "<D> | ";
                break;
            case severity_type::warning:
                log_stream << "<W> | ";
                break;
            case severity_type::error:
                log_stream << "<E> | ";
                break;
        };

        print_impl(args...);
    }

    ~logger()
    {
        if (policy)
        {
            policy->close_ostream();
            delete policy;
        }
    }

private:
    std::string get_time()
    {
        time_t raw_time;
        if (time(&raw_time) != static_cast<time_t>(-1))
        {
            std::string time_str = ctime(&raw_time);
            return time_str.substr(0, time_str.size() - 1); // strip newline
        }
        else
        {
            return "UNKNOWN";
        }
    }

    std::string get_logline_header()
    {
        std::stringstream header;

        header << "[" << get_time() << " - ";
        header.fill('0');
        header.width(7);
        header << clock() << "] ~ ";

        return header.str();
    }

    void print_impl()
    {
        policy->write(get_logline_header() + log_stream.str());
        log_stream.str("");
    }

    template<typename First, typename ...Rest>
    void print_impl(First first, Rest ...rest)
    {
        log_stream << first;
        print_impl(rest...);
    }

    std::mutex write_lock;
    std::stringstream log_stream;
    Policy *policy;
};

// static logger<file_log_policy> _logger("execution.log");
static logger<std_log_policy> _logger;

#ifdef LOGGING_ENABLED
#define LOG _logger.print<severity_type::debug>
#define LOG_ERROR _logger.print<severity_type::error>
#define LOG_WARNING _logger.print<severity_type::warning>
#else
#define LOG(...)
#define LOG_ERROR(...)
#define LOG_WARNING(...)
#endif

#endif
