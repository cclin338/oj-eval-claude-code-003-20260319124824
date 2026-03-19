#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;

    Submission(string p, string s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    bool solved;
    int solved_time;
    int wrong_attempts;
    int freeze_submissions;

    ProblemStatus() : solved(false), solved_time(0), wrong_attempts(0), freeze_submissions(0) {}
};

struct TeamInfo {
    string name;
    int solved_count;
    int total_penalty;
    vector<int> solved_times;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;

    TeamInfo(string n) : name(n), solved_count(0), total_penalty(0) {}
};

class ICPMSystem {
private:
    bool competition_started;
    bool frozen;
    int duration_time;
    int problem_count;
    int freeze_time;
    map<string, TeamInfo*> teams;
    vector<TeamInfo*> teams_order;
    vector<string> problem_names;

    void updateRankings() {
        sort(teams_order.begin(), teams_order.end(), [this](TeamInfo* a, TeamInfo* b) {
            if (a->solved_count != b->solved_count) {
                return a->solved_count > b->solved_count;
            }

            if (a->total_penalty != b->total_penalty) {
                return a->total_penalty < b->total_penalty;
            }

            vector<int>& times_a = a->solved_times;
            vector<int>& times_b = b->solved_times;
            int n = min(times_a.size(), times_b.size());

            for (int i = 0; i < n; i++) {
                if (times_a[i] != times_b[i]) {
                    return times_a[i] < times_b[i];
                }
            }

            return a->name < b->name;
        });
    }

    int getRanking(TeamInfo* team) {
        for (int i = 0; i < teams_order.size(); i++) {
            if (teams_order[i] == team) {
                return i + 1;
            }
        }
        return -1;
    }

    string getProblemDisplay(const ProblemStatus& ps, bool frozen_state) {
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
    ICPMSystem() : competition_started(false), frozen(false), duration_time(0), problem_count(0), freeze_time(-1) {}

    bool addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started." << endl;
            return false;
        }

        if (teams.count(team_name) > 0) {
            cout << "[Error]Add failed: duplicated team name." << endl;
            return false;
        }

        TeamInfo* team = new TeamInfo(team_name);
        teams[team_name] = team;
        teams_order.push_back(team);

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

        problem_names.clear();
        for (int i = 0; i < problems; i++) {
            problem_names.push_back(string(1, 'A' + i));
        }

        cout << "[Info]Competition starts." << endl;
        return true;
    }

    void submit(const string& problem, const string& team_name, const string& status, int time) {
        if (teams.count(team_name) == 0) return;

        TeamInfo* team = teams[team_name];
        team->submissions.push_back(Submission(problem, status, time));

        ProblemStatus& ps = team->problems[problem];

        if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solved_time = time;
                team->solved_count++;
                team->total_penalty += 20 * ps.wrong_attempts + time;

                // Insert solved time in sorted order
                vector<int>::iterator it = upper_bound(team->solved_times.begin(), team->solved_times.end(), time);
                team->solved_times.insert(it, time);
            } else {
                if (frozen && time > freeze_time) {
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
        freeze_time = 0; // Mark that freeze has happened - submissions after this will be frozen
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

        // Process scrolling - repeatedly find lowest ranked team with frozen problems
        bool has_frozen = true;
        while (has_frozen) {
            has_frozen = false;

            // Find lowest ranked team with frozen problems
            TeamInfo* lowest_team = nullptr;
            string smallest_frozen_prob;

            for (auto it = teams_order.rbegin(); it != teams_order.rend(); ++it) {
                TeamInfo* team = *it;

                for (const string& prob : problem_names) {
                    const ProblemStatus& ps = team->problems[prob];
                    if (!ps.solved && ps.freeze_submissions > 0) {
                        lowest_team = team;
                        smallest_frozen_prob = prob;
                        break;
                    }
                }

                if (lowest_team) break;
            }

            if (!lowest_team) break; // No more teams with frozen problems

            has_frozen = true;

            // Unfreeze the smallest problem for this team
            ProblemStatus& ps = lowest_team->problems[smallest_frozen_prob];
            ps.wrong_attempts += ps.freeze_submissions;
            ps.freeze_submissions = 0;

            // Process submissions that occurred during freeze
            int old_rank = getRanking(lowest_team);
            int old_solved = lowest_team->solved_count;
            int old_penalty = lowest_team->total_penalty;

            // Re-process all submissions for this problem during freeze
            for (const Submission& sub : lowest_team->submissions) {
                if (sub.problem == smallest_frozen_prob && sub.time > freeze_time) {
                    if (!ps.solved && sub.status == "Accepted") {
                        ps.solved = true;
                        ps.solved_time = sub.time;
                        lowest_team->solved_count++;
                        lowest_team->total_penalty += 20 * ps.wrong_attempts + sub.time;

                        // Insert in sorted order
                        vector<int>::iterator it = upper_bound(lowest_team->solved_times.begin(), lowest_team->solved_times.end(), sub.time);
                        lowest_team->solved_times.insert(it, sub.time);
                    } else if (!ps.solved && sub.status != "Accepted") {
                        ps.wrong_attempts++;
                    }
                }
            }

            // Update rankings and check if this caused a change
            vector<string> old_order;
            for (TeamInfo* t : teams_order) {
                old_order.push_back(t->name);
            }

            updateRankings();
            int new_rank = getRanking(lowest_team);

            if (new_rank != old_rank) {
                // The displaced team is the one that was pushed down
                // Find the team directly below lowest_team in the new ranking
                // that was above it in the old ranking

                // Find lowest team's position in old order
                int old_pos = -1;
                for (int i = 0; i < old_order.size(); i++) {
                    if (old_order[i] == lowest_team->name) {
                        old_pos = i;
                        break;
                    }
                }

                // Find lowest team's position in new order
                int new_pos = -1;
                for (int i = 0; i < teams_order.size(); i++) {
                    if (teams_order[i]->name == lowest_team->name) {
                        new_pos = i;
                        break;
                    }
                }

                // The displaced team is the one immediately above in new order
                // that was below in old order
                if (new_pos > 0) {
                    TeamInfo* displaced_team = teams_order[new_pos - 1];
                    // Check if this team was actually ranked lower before
                    int displaced_old_rank = -1;
                    for (int i = 0; i < old_order.size(); i++) {
                        if (old_order[i] == displaced_team->name) {
                            displaced_old_rank = i + 1;
                            break;
                        }
                    }

                    if (displaced_old_rank > old_rank) {
                        cout << lowest_team->name << " " << displaced_team->name << " " << lowest_team->solved_count << " " << lowest_team->total_penalty << endl;
                    }
                }
            }
        }

        frozen = false;

        // Print final scoreboard after scrolling
        printScoreboard();

        return true;
    }

    void queryRanking(const string& team_name) {
        if (teams.count(team_name) == 0) {
            cout << "[Error]Query ranking failed: cannot find the team." << endl;
            return;
        }

        TeamInfo* team = teams[team_name];
        int rank = getRanking(team);

        cout << "[Info]Complete query ranking." << endl;
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
        }
        cout << team_name << " NOW AT RANKING " << rank << endl;
    }

    void querySubmission(const string& team_name, const string& problem, const string& status) {
        if (teams.count(team_name) == 0) {
            cout << "[Error]Query submission failed: cannot find the team." << endl;
            return;
        }

        cout << "[Info]Complete query submission." << endl;

        TeamInfo* team = teams[team_name];
        Submission* last_submission = nullptr;

        for (int i = team->submissions.size() - 1; i >= 0; i--) {
            Submission& sub = team->submissions[i];
            bool problem_match = (problem == "ALL" || sub.problem == problem);
            bool status_match = (status == "ALL" || sub.status == status);

            if (problem_match && status_match) {
                last_submission = &sub;
                break;
            }
        }

        if (last_submission == nullptr) {
            cout << "Cannot find any submission." << endl;
        } else {
            cout << team_name << " " << last_submission->problem << " " << last_submission->status << " " << last_submission->time << endl;
        }
    }

    void endCompetition() {
        cout << "[Info]Competition ends." << endl;
    }

    void printScoreboard() {
        for (TeamInfo* team : teams_order) {
            cout << team->name << " " << getRanking(team) << " " << team->solved_count << " " << team->total_penalty;

            for (const string& prob : problem_names) {
                cout << " ";
                const ProblemStatus& ps = team->problems[prob];
                cout << getProblemDisplay(ps, frozen);
            }

            cout << endl;
        }
    }
};

int main() {
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