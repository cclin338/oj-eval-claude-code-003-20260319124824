#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    bool frozen;

    Submission(string p, string s, int t, bool f = false) : problem(p), status(s), time(t), frozen(f) {}
};

struct ProblemStatus {
    bool solved;
    int solved_time;
    int wrong_attempts;
    int freeze_submissions;
    int last_submission_time;

    ProblemStatus() : solved(false), solved_time(0), wrong_attempts(0), freeze_submissions(0), last_submission_time(0) {}
};

struct TeamInfo {
    string name;
    int index; // Original team order
    int solved_count;
    int total_penalty;
    vector<int> solved_times;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;

    TeamInfo(string n, int idx) : name(n), index(idx), solved_count(0), total_penalty(0) {}
};

class ICPMSystem {
private:
    bool competition_started;
    bool frozen;
    int duration_time;
    int problem_count;
    int freeze_time;
    int current_time;
    map<string, int> team_indices;
    vector<TeamInfo> teams;
    vector<int> team_rank; // team_rank[i] = current rank of team at index i

    void updateRankings() {
        if (teams.empty()) return;

        // Create vector of team indices to sort
        vector<int> sorted_indices(teams.size());
        for (int i = 0; i < teams.size(); i++) {
            sorted_indices[i] = i;
        }

        // Sort by ICPC rules
        sort(sorted_indices.begin(), sorted_indices.end(), [this](int a, int b) {
            const TeamInfo& ta = teams[a];
            const TeamInfo& tb = teams[b];

            if (ta.solved_count != tb.solved_count) {
                return ta.solved_count > tb.solved_count;
            }

            if (ta.total_penalty != tb.total_penalty) {
                return ta.total_penalty < tb.total_penalty;
            }

            // Compare solved times in reverse order
            int n = min(ta.solved_times.size(), tb.solved_times.size());
            for (int i = n - 1; i >= 0; i--) {
                if (ta.solved_times[i] != tb.solved_times[i]) {
                    return ta.solved_times[i] < tb.solved_times[i];
                }
            }

            return ta.name < tb.name;
        });

        // Update team_rank array
        for (int i = 0; i < sorted_indices.size(); i++) {
            team_rank[sorted_indices[i]] = i + 1;
        }

        // Update teams_order based on sorted indices
        vector<int> new_order;
        for (int idx : sorted_indices) {
            new_order.push_back(idx);
        }

        // Update original order if needed (teams vector remains unchanged)
    }

    int getRankByIndex(int idx) const {
        return team_rank[idx];
    }

    string getProblemDisplay(const ProblemStatus& ps, bool frozen_state) const {
        if (ps.solved) {
            if (ps.wrong_attempts == 0) {
                return "+";
            } else {
                return "+" + to_string(ps.wrong_attempts);
            }
        } else {
            if (frozen_state && ps.freeze_submissions > 0) {
                if (ps.wrong_attempts == 0) {
                    return "0/" + to_string(ps.freeze_submissions);
                } else {
                    return "-" + to_string(ps.wrong_attempts) + "/" + to_string(ps.freeze_submissions);
                }
            } else {
                int total_wrong = ps.wrong_attempts;
                if (total_wrong == 0) {
                    return ".";
                } else {
                    return "-" + to_string(total_wrong);
                }
            }
        }
    }

public:
    ICPMSystem() : competition_started(false), frozen(false), duration_time(0), problem_count(0), freeze_time(-1), current_time(0) {}

    bool addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started." << endl;
            return false;
        }

        if (team_indices.count(team_name) > 0) {
            cout << "[Error]Add failed: duplicated team name." << endl;
            return false;
        }

        int idx = teams.size();
        team_indices[team_name] = idx;
        teams.emplace_back(team_name, idx);
        team_rank.push_back(0);

        cout << "[Info]Add successfully." << endl;
        return true;
    }

    bool startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started." << endl;
            return false;
        }

        competition_started = true;
        duration_time = duration;
        problem_count = problems;
        current_time = 0;

        cout << "[Info]Competition starts." << endl;
        return true;
    }

    void submit(const string& problem, const string& team_name, const string& status, int time) {
        if (team_indices.count(team_name) == 0) return;

        int team_idx = team_indices[team_name];
        current_time = max(current_time, time);

        TeamInfo& team = teams[team_idx];
        team.submissions.push_back(Submission(problem, status, time, frozen));

        ProblemStatus& ps = team.problems[problem];

        if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solved_time = time;
                team.solved_count++;
                team.total_penalty += 20 * ps.wrong_attempts + time;

                // Insert solved time in sorted order (descending for easy comparison)
                auto it = upper_bound(team.solved_times.begin(), team.solved_times.end(), time);
                team.solved_times.insert(it, time);
            } else {
                if (frozen && freeze_time >= 0) {
                    ps.freeze_submissions++;
                } else {
                    ps.wrong_attempts++;
                }
            }
        }
    }

    void flushScoreboard() {
        updateRankings();
        cout << "[Info]Flush scoreboard." << endl;
    }

    bool freezeScoreboard() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
            return false;
        }

        frozen = true;
        freeze_time = current_time;
        cout << "[Info]Freeze scoreboard." << endl;
        return true;
    }

    bool scrollScoreboard() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
            return false;
        }

        // First flush
        updateRankings();

        cout << "[Info]Scroll scoreboard." << endl;

        // Print scoreboard before scrolling
        printScoreboard();

        // Process scrolling
        vector<pair<int, int>> ranking_changes;

        // Find all teams with frozen problems
        vector<int> teams_with_frozen;
        for (int i = 0; i < teams.size(); i++) {
            bool has_frozen = false;
            for (const auto& prob : teams[i].problems) {
                if (!prob.second.solved && prob.second.freeze_submissions > 0) {
                    has_frozen = true;
                    break;
                }
            }
            if (has_frozen) {
                teams_with_frozen.push_back(i);
            }
        }

        // Store old rankings before any changes
        vector<int> old_rankings = team_rank;

        while (!teams_with_frozen.empty()) {
            // Find lowest ranked team with frozen problems
            int lowest_idx = -1;
            int lowest_rank = teams.size() + 1;

            for (int idx : teams_with_frozen) {
                if (getRankByIndex(idx) < lowest_rank) {
                    lowest_rank = getRankByIndex(idx);
                    lowest_idx = idx;
                }
            }

            if (lowest_idx == -1) break;

            TeamInfo& lowest_team = teams[lowest_idx];
            string smallest_prob = "";

            // Find smallest problem with frozen submissions
            for (int i = 0; i < problem_count; i++) {
                const string& prob = string(1, 'A' + i);
                const ProblemStatus& ps = lowest_team.problems[prob];
                if (!ps.solved && ps.freeze_submissions > 0) {
                    smallest_prob = prob;
                    break;
                }
            }

            if (smallest_prob.empty()) {
                auto it = find(teams_with_frozen.begin(), teams_with_frozen.end(), lowest_idx);
                if (it != teams_with_frozen.end()) {
                    teams_with_frozen.erase(it);
                }
                continue;
            }

            // Unfreeze this problem
            ProblemStatus& ps = lowest_team.problems[smallest_prob];
            ps.wrong_attempts += ps.freeze_submissions;
            ps.freeze_submissions = 0;

            int old_solved = lowest_team.solved_count;
            int old_penalty = lowest_team.total_penalty;

            // Process all submissions after freeze time for this problem
            for (const Submission& sub : lowest_team.submissions) {
                if (sub.problem == smallest_prob && sub.time > freeze_time) {
                    if (!ps.solved && sub.status == "Accepted") {
                        ps.solved = true;
                        ps.solved_time = sub.time;
                        lowest_team.solved_count++;
                        lowest_team.total_penalty += 20 * ps.wrong_attempts + sub.time;

                        auto it = upper_bound(lowest_team.solved_times.begin(), lowest_team.solved_times.end(), sub.time);
                        lowest_team.solved_times.insert(it, sub.time);
                    } else if (!ps.solved && sub.status != "Accepted") {
                        ps.wrong_attempts++;
                    }
                }
            }

            // Update rankings
            int old_rank = old_rankings[lowest_idx];
            updateRankings();
            int new_rank = getRankByIndex(lowest_idx);

            // Record if ranking changed
            if (new_rank < old_rank) {
                ranking_changes.emplace_back(lowest_idx, old_rank);
            }

            // Update teams_with_frozen
            teams_with_frozen.clear();
            for (int i = 0; i < teams.size(); i++) {
                bool has_frozen = false;
                for (const auto& prob : teams[i].problems) {
                    if (!prob.second.solved && prob.second.freeze_submissions > 0) {
                        has_frozen = true;
                        break;
                    }
                }
                if (has_frozen) {
                    teams_with_frozen.push_back(i);
                }
            }

            // Store current rankings for next iteration
            old_rankings = team_rank;
        }

        frozen = false;

        // Print all ranking changes
        for (const auto& change : ranking_changes) {
            int team_idx = change.first;
            int old_rank = change.second;
            int new_rank = getRankByIndex(team_idx);

            if (new_rank < old_rank) { // Team moved up
                // Find the team that was displaced (should be at old_rank now)
                int displaced_idx = -1;
                for (int i = 0; i < teams.size(); i++) {
                    if (i != team_idx && getRankByIndex(i) == old_rank) {
                        displaced_idx = i;
                        break;
                    }
                }

                if (displaced_idx != -1 && old_rankings[displaced_idx] == new_rank) {
                    cout << teams[team_idx].name << " " << teams[displaced_idx].name << " "
                         << teams[team_idx].solved_count << " " << teams[team_idx].total_penalty << endl;
                }
            }
        }

        // Print final scoreboard after scrolling
        printScoreboard();

        return true;
    }

    void queryRanking(const string& team_name) {
        if (team_indices.count(team_name) == 0) {
            cout << "[Error]Query ranking failed: cannot find the team." << endl;
            return;
        }

        int team_idx = team_indices[team_name];
        int rank = getRankByIndex(team_idx);

        cout << "[Info]Complete query ranking." << endl;
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
        }
        cout << team_name << " NOW AT RANKING " << rank << endl;
    }

    void querySubmission(const string& team_name, const string& problem, const string& status) {
        if (team_indices.count(team_name) == 0) {
            cout << "[Error]Query submission failed: cannot find the team." << endl;
            return;
        }

        cout << "[Info]Complete query submission." << endl;

        int team_idx = team_indices[team_name];
        const TeamInfo& team = teams[team_idx];

        // Find last submission matching criteria
        for (int i = team.submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = team.submissions[i];
            bool problem_match = (problem == "ALL" || sub.problem == problem);
            bool status_match = (status == "ALL" || sub.status == status);

            if (problem_match && status_match) {
                cout << team_name << " " << sub.problem << " " << sub.status << " " << sub.time << endl;
                return;
            }
        }

        cout << "Cannot find any submission." << endl;
    }

    void endCompetition() {
        cout << "[Info]Competition ends." << endl;
    }

    void printScoreboard() {
        // Sort by current ranking
        vector<int> sorted_teams;
        for (int i = 0; i < teams.size(); i++) {
            sorted_teams.push_back(i);
        }
        sort(sorted_teams.begin(), sorted_teams.end(), [this](int a, int b) {
            return getRankByIndex(a) < getRankByIndex(b);
        });

        for (int idx : sorted_teams) {
            const TeamInfo& team = teams[idx];
            cout << team.name << " " << getRankByIndex(idx) << " " << team.solved_count << " " << team.total_penalty;

            for (int i = 0; i < problem_count; i++) {
                auto it = team.problems.find(string(1, 'A' + i));
                const ProblemStatus& ps = (it != team.problems.end()) ? it->second : ProblemStatus();
                cout << " " << getProblemDisplay(ps, frozen);
            }

            cout << endl;
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPMSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string command;
        iss >> command;

        if (command == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.addTeam(team_name);
        } else if (command == "START") {
            string duration_str, duration_val, problem_str, problem_count;
            iss >> duration_str >> duration_val >> problem_str >> problem_count;
            int duration = stoi(duration_val);
            int problems = stoi(problem_count);
            system.startCompetition(duration, problems);
        } else if (command == "SUBMIT") {
            string problem, by_str, team_name, with_str, status, at_str;
            int time;
            iss >> problem >> by_str >> team_name >> with_str >> status >> at_str >> time;
            system.submit(problem, team_name, status, time);
        } else if (command == "FLUSH") {
            system.flushScoreboard();
        } else if (command == "FREEZE") {
            system.freezeScoreboard();
        } else if (command == "SCROLL") {
            system.scrollScoreboard();
        } else if (command == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.queryRanking(team_name);
        } else if (command == "QUERY_SUBMISSION") {
            string team_name;
            getline(iss >> ws, team_name); // Read team name and the rest

            size_t problem_pos = team_name.find("PROBLEM=");
            size_t status_pos = team_name.find("STATUS=");

            if (problem_pos != string::npos && status_pos != string::npos) {
                string problem = team_name.substr(problem_pos + 8, status_pos - problem_pos - 9);
                string status = team_name.substr(status_pos + 7);

                // Trim any trailing whitespace
                size_t end_pos = status.find_last_not_of(" \t\r\n");
                if (end_pos != string::npos) {
                    status = status.substr(0, end_pos + 1);
                }

                // Extract just the team name
                team_name = team_name.substr(0, team_name.find(" WHERE"));

                system.querySubmission(team_name, problem, status);
            }
        } else if (command == "END") {
            system.endCompetition();
            break;
        }
    }

    return 0;
}