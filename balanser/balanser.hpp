#include <crow.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>

struct LoadBalancerConfig {
    std::string algorithm_ = "round_robin"; 
    int health_check_interval_ = 30; 
    int session_timeout_ = 300; 
    bool sticky_sessions_ = true;
};

struct Server {
    std::string id_;
    std::string host_;
    int port_;
    bool is_healthy_;
    int active_connections_;
    int weight_; 
    std::chrono::steady_clock::time_point last_health_check_;
    
    bool CheckHealth();
};

class LoadBalancingAlgorithm {
public:
    virtual ~LoadBalancingAlgorithm() = default;
    virtual Server* SelectServer(const std::string& client_ip = "") = 0;
    virtual void UpdateServerStats(const std::string& server_id, int connections) = 0;
};

class RoundRobinAlgorithm : public LoadBalancingAlgorithm {
private:
    std::vector<Server*> servers_;
    size_t current_index_;
    
public:
    RoundRobinAlgorithm(const std::vector<Server*>& servers);
    Server* SelectServer(const std::string& client_ip = "") override;
    void UpdateServerStats(const std::string& server_id, int connections) override;
};

class LeastConnectionsAlgorithm : public LoadBalancingAlgorithm {
private:
    std::vector<Server*> servers_;
    
public:
    LeastConnectionsAlgorithm(const std::vector<Server*>& servers);
    Server* SelectServer(const std::string& client_ip = "") override;
    void UpdateServerStats(const std::string& server_id, int connections) override;
};

class IPHashAlgorithm : public LoadBalancingAlgorithm {
private:
    std::vector<Server*> servers_;
    
public:
    IPHashAlgorithm(const std::vector<Server*>& servers);
    Server* SelectServer(const std::string& client_ip = "") override;
    void UpdateServerStats(const std::string& server_id, int connections) override;
};

class LoadBalancer {
private:
    LoadBalancerConfig config_;
    std::vector<std::unique_ptr<Server>> backend_servers_;
    std::unique_ptr<LoadBalancingAlgorithm> algorithm_;
    std::map<std::string, std::string> session_map_; 
    
    void HealthCheckWorker();
    std::thread health_check_thread_;
    bool running_;
    
public:
    LoadBalancer(const LoadBalancerConfig& config);
    ~LoadBalancer();
    
    bool AddServer(const std::string& host, int port, int weight = 1);
    bool RemoveServer(const std::string& server_id);
    
    crow::response RouteRequest(const crow::request& req);
    
    std::string GetServerForSession(const std::string& session_id);
    void BindSessionToServer(const std::string& session_id, const std::string& server_id);
    
    void PrintStats();
    
private:
    std::string GetClientIP(const crow::request& req);
    void ForwardRequest(const crow::request& req, crow::response& res, Server* server);
};

class SessionAwareMiddleware {
private:
    LoadBalancer& load_balancer_;
    
public:
    SessionAwareMiddleware(LoadBalancer& lb) : load_balancer_(lb) {}
    
    void before_handle(crow::request& req, crow::response& res, context& ctx);
    void after_handle(crow::request& req, crow::response& res, context& ctx);
};
