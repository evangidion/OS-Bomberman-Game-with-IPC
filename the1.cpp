#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
extern "C" {
	#include "logging.h"
}
extern "C" {
	#include "message.h"
}
extern "C" {
	#include "message.c"
}
extern "C" {
	#include "logging.c"
}
#include <sys/socket.h>
#include <vector>
#include <cstring>
#include <string.h>
#include <poll.h>
#include <cstdlib>

#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

using namespace std;

typedef struct obstacle {
	bool real;
	int loc_x;
	int loc_y;
	int dur;
} obstacle;

typedef struct bomber {
	bool dead;
	bool real;
	pid_t pid;
	int fd;
	int loc_x;
	int loc_y;
	bool winner;
	bool reaped;
} bomber;

typedef struct bomb {
	bool real;
	pid_t pid;
	int loc_x;
	int loc_y;
	int fd;
	int r;
} bomb;

vector<obstacle> obs;
vector<bomber> bombers;
vector<bomb> bombs;
int stat;

bomber GetBomberAtCoordinate(coordinate coordinate) {
	for(bomber bomber : bombers) {
		if(bomber.loc_x == coordinate.x && bomber.loc_y == coordinate.y && !bomber.dead) return bomber;
	}
	bomber error;
	error.real = false;
	return error;
}

bomb GetBombAtCoordinate(coordinate coordinate) {
	for(bomb bomb : bombs) {
		if(bomb.loc_x == coordinate.x && bomb.loc_y == coordinate.y) return bomb;
	}
	bomb error;
	error.real = false;
	return error;
}

obstacle GetObstacleAtCoordinate(coordinate coordinate) {
	for(obstacle obstacle : obs) {
		if(obstacle.loc_x == coordinate.x && obstacle.loc_y == coordinate.y) return obstacle;
	}
	obstacle error;
	error.real = false;
	return error;
}

bool getBomber(coordinate c) {
	for(bomber bomber : bombers) {
		if(bomber.loc_x == c.x && bomber.loc_y == c.y && !bomber.dead) return true;
	}
	return false;
}

bool getBomb(coordinate c) {
	for(bomb bomb : bombs) {
		if(bomb.loc_x == c.x && bomb.loc_y == c.y) return true;
	}
	return false;
}

bool getObstacle(coordinate c) {
	for(obstacle obstacle : obs) {
		if(obstacle.loc_x == c.x && obstacle.loc_y == c.y) return true;
	}
	return false;
}

int absolute(int a, int b) {
	return a - b > 0 ? a - b : b - a;
}

bool AllBombersReaped()
{
	for(bomber b : bombers)
	{
		if(!b.reaped) return false;
	}
	return true;
}



int GetAliveBombersCount()
{
	int result = 0;
	for(bomber b : bombers)
	{
		if(!b.dead) result++;
	}
	return result;
}


int main() { 
	int x, y, o_count, bomber_count;
	cin >> x >> y >> o_count >> bomber_count;

	for(int i = 0; i < o_count; i++) {
		obstacle o;
		int loc_x, loc_y, dur;
		cin >> loc_x >> loc_y >> dur;
		o.loc_x = loc_x;
		o.loc_y = loc_y;
		o.dur = dur;
		o.real = true;
		obs.push_back(o);
	}
	for(int i = 0; i < bomber_count; i++)
	{
		int fd[2];
		PIPE(fd);
		bomber b;
		int loc_x, loc_y, c;
		cin >> loc_x >> loc_y >> c;
		b.loc_x = loc_x;
		b.loc_y = loc_y;
		b.fd = fd[0];
		b.dead = false;
		b.real = true;
		b.winner = false;
		b.reaped = false;
		char* arguments[c + 1];
		string arg;
		string execname;
		cin >> execname;
		arguments[0] = new char[execname.length() + 1];
		strcpy(arguments[0], execname.c_str());
		for(int j = 1; j < c; j++) {
			cin >> arg;
			arguments[j] = new char[arg.length() + 1];
			strcpy(arguments[j], arg.c_str());
		}
		arguments[c] = NULL;
		pid_t pid = fork();
		if(pid == 0) {
			dup2(fd[1], 0);
			dup2(fd[1], 1);
			close(fd[0]);
			close(fd[1]);
			execv(arguments[0], arguments);
		}
		b.pid = pid;
		bombers.push_back(b);
		close(fd[1]);
		for(int j = 0; j < c + 1; j++) {
			delete [] arguments[j];
		}
	}
	if(bomber_count > 1) {
		while(!(AllBombersReaped() && bombs.size() == 0))
		{
			int pollCount = bombers.size() + bombs.size();
			//struct pollfd* pfds = new pollfd[pollCount];
			struct pollfd pfds[pollCount];
			for(int i = 0; i < bombs.size(); i++) {
				pfds[i].fd = bombs[i].fd;
				pfds[i].events = POLLIN;
			}
			for(int i = 0; i < bombers.size(); i++) {
				pfds[i + bombs.size()].fd = bombers[i].fd;
				pfds[i + bombs.size()].events = POLLIN;
			}
			poll(pfds, pollCount, 0);
			for(int i = 0; i < pollCount; i++) {
				if(pfds[i].revents & POLLIN) {
					for(int j = 0; j < bombs.size(); j++) {
						if(pfds[i].fd == bombs[j].fd) {
							im* i_message = new im;
							read_data(bombs[j].fd, i_message);
							imp *in = new imp;
							in->pid = bombs[j].pid;
							in->m = i_message;
							print_output(in, NULL, NULL, NULL);
							coordinate c;
							c.x = bombs[j].loc_x;
							c.y = bombs[j].loc_y;
							bomber b = GetBomberAtCoordinate(c);
							vector<bomber> tempbombers;
							delete i_message;
							delete in;
							if(b.real) {
								for(int k = 0; k < bombers.size(); k++) {
									if(bombers[k].loc_x == c.x && bombers[k].loc_y == c.y) {
										bombers[k].dead = true;
										tempbombers.push_back(b);
										break;
									}
								}
							}
							int r = bombs[j].r;
							obstacle o;
							obsd* obsta = new obsd;
							for(int k = 1; k < r + 1; k++) {
								if(bombs[j].loc_x - k < 0) break;
								c.x = bombs[j].loc_x - k;
								o = GetObstacleAtCoordinate(c);
								if(o.real) {
									for(int l = 0; l < obs.size(); l++) {
										if(obs[l].loc_x == c.x && obs[l].loc_y == c.y) {
											if(obs[l].dur != -1) {
												if(obs[l].dur == 1) {
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = 0;
													print_output(NULL, NULL, obsta, NULL);
													obs.erase(obs.begin() + l);
												}
												else {
													obs[l].dur--;
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = obs[l].dur;
													print_output(NULL, NULL, obsta, NULL);
												}
											}
											else {
												obsta->position.x = obs[l].loc_x;
												obsta->position.y = obs[l].loc_y;
												obsta->remaining_durability = -1;
												print_output(NULL, NULL, obsta, NULL);
											}
											break;
										}
									}
									break;
								}
								b = GetBomberAtCoordinate(c);
								if(b.real) {
									for(int l = 0; l < bombers.size(); l++) {
										if(bombers[l].loc_x == c.x && bombers[l].loc_y == c.y) {
											bombers[l].dead = true;
											tempbombers.push_back(b);
											break;
										}
									}
								}
							}
							
							for(int k = 1; k < r + 1; k++) {
								if(bombs[j].loc_x + k >= x) break;
								c.x = bombs[j].loc_x + k;
								o = GetObstacleAtCoordinate(c);
								if(o.real) {
									for(int l = 0; l < obs.size(); l++) {
										if(obs[l].loc_x == c.x && obs[l].loc_y == c.y) {
											if(obs[l].dur != -1) {
												if(obs[l].dur == 1) {
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = 0;
													print_output(NULL, NULL, obsta, NULL);
													obs.erase(obs.begin() + l);
												}
												else {
													obs[l].dur--;
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = obs[l].dur;
													print_output(NULL, NULL, obsta, NULL);
												}
											}
											else {
												obsta->position.x = obs[l].loc_x;
												obsta->position.y = obs[l].loc_y;
												obsta->remaining_durability = -1;
												print_output(NULL, NULL, obsta, NULL);
											}
											break;
										}
									}
									break;
								}
								b = GetBomberAtCoordinate(c);
								if(b.real) {
									for(int l = 0; l < bombers.size(); l++) {
										if(bombers[l].loc_x == c.x && bombers[l].loc_y == c.y) {
											bombers[l].dead = true;
											tempbombers.push_back(b);
											break;
										}
									}
								}
							}
							
							c.x = bombs[j].loc_x;
							for(int k = 1; k < r + 1; k++) {
								if(bombs[j].loc_y - k < 0) break;
								c.y = bombs[j].loc_y - k;
								o = GetObstacleAtCoordinate(c);
								if(o.real) {
									for(int l = 0; l < obs.size(); l++) {
										if(obs[l].loc_x == c.x && obs[l].loc_y == c.y) {
											if(obs[l].dur != -1) {
												if(obs[l].dur == 1) {
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = 0;
													print_output(NULL, NULL, obsta, NULL);
													obs.erase(obs.begin() + l);
												}
												else {
													obs[l].dur--;
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = obs[l].dur;
													print_output(NULL, NULL, obsta, NULL);
												}
											}
											else {
												obsta->position.x = obs[l].loc_x;
												obsta->position.y = obs[l].loc_y;
												obsta->remaining_durability = -1;
												print_output(NULL, NULL, obsta, NULL);
											}
											break;
										}
									}
									break;
								}
								b = GetBomberAtCoordinate(c);
								if(b.real) {
									for(int l = 0; l < bombers.size(); l++) {
										if(bombers[l].loc_x == c.x && bombers[l].loc_y == c.y) {
											bombers[l].dead = true;
											tempbombers.push_back(b);
											break;
										}
									}
								}
							}
							
							for(int k = 1; k < r + 1; k++) {
								if(bombs[j].loc_y + k >= y) break;
								c.y = bombs[j].loc_y + k;
								o = GetObstacleAtCoordinate(c);
								if(o.real) {
									for(int l = 0; l < obs.size(); l++) {
										if(obs[l].loc_x == c.x && obs[l].loc_y == c.y) {
											if(obs[l].dur != -1) {
												if(obs[l].dur == 1) {
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = 0;
													print_output(NULL, NULL, obsta, NULL);
													obs.erase(obs.begin() + l);
												}
												else {
													obs[l].dur--;
													obsta->position.x = obs[l].loc_x;
													obsta->position.y = obs[l].loc_y;
													obsta->remaining_durability = obs[l].dur;
													print_output(NULL, NULL, obsta, NULL);
												}
											}
											else {
												obsta->position.x = obs[l].loc_x;
												obsta->position.y = obs[l].loc_y;
												obsta->remaining_durability = -1;
												print_output(NULL, NULL, obsta, NULL);
											}
											break;
										}
									}
									break;
								}
								b = GetBomberAtCoordinate(c);
								if(b.real) {
									for(int l = 0; l < bombers.size(); l++) {
										if(bombers[l].loc_x == c.x && bombers[l].loc_y == c.y) {
											bombers[l].dead = true;
											tempbombers.push_back(b);
											break;
										}
									}
								}
							}
							delete obsta;
							if(GetAliveBombersCount() == 0) {
								if(tempbombers.size()) {
									bomber won = tempbombers[0];
									int distance = absolute(won.loc_x, bombs[j].loc_x) + absolute(won.loc_y, bombs[j].loc_y);
									for(int k = 1; k < tempbombers.size(); k++) {
										if(absolute(tempbombers[k].loc_x, bombs[j].loc_x) + absolute(tempbombers[k].loc_y, bombs[j].loc_y) > distance) {
											won = tempbombers[k];
											distance = absolute(tempbombers[k].loc_x, bombs[j].loc_x) + absolute(tempbombers[k].loc_y, bombs[j].loc_y);
										}
									}
									for(int k = 0; k < bombers.size(); k++) {
										if(won.pid == bombers[k].pid) {
											bombers[k].winner = true;
											break;
										}
									}
								}
							}
							if(GetAliveBombersCount() == 1) {
								for(int k = 0; k < bombers.size(); k++) {
									if(!bombers[k].dead) {
										bombers[k].winner = true;
										break;
									}
								}
							}
							close(bombs[j].fd);
							waitpid(bombs[j].pid, &stat, 0);
							bombs.erase(bombs.begin() + j);

						}
					}
				}
			}

			for(int i = 0; i < pollCount; i++)
			{
				if(pfds[i].revents & POLLIN)
				{
					for(int j = 0; j < bombers.size();j++)
					{
						if(pfds[i].fd == bombers[j].fd)
						{
							im* i_message = new im;
							read_data(bombers[j].fd, i_message);
							imp* in = new imp;
							in->pid = bombers[j].pid;
							in->m = i_message;
							print_output(in, NULL, NULL, NULL);
							om* o_message = new om;
							omp* out = new omp;
							out->pid = bombers[j].pid;
							delete in;
							if(GetAliveBombersCount() > 1 && !bombers[j].dead) {
								switch(i_message->type) {
									case BOMBER_START:
									{
										o_message->type = BOMBER_LOCATION;
										o_message->data.new_position.x = bombers[j].loc_x;
										o_message->data.new_position.y = bombers[j].loc_y;
										send_message(bombers[j].fd, o_message);
										out->m = o_message;
										print_output(NULL, out, NULL, NULL);
										delete i_message;
										delete o_message;
										delete out;
										break;
									}
									
									case BOMBER_SEE:
									{
										o_message->type = BOMBER_VISION;
										int count = 0;
										coordinate c;
										c.x = bombers[j].loc_x;
										c.y = bombers[j].loc_y;
										bomb b = GetBombAtCoordinate(c);
										bomber bomberr;
										vector<obstacle> tempobs;
										vector<bomb> tempbombs;
										vector<bomber> tempbombers;
										if(b.real) {
											count++;
											tempbombs.push_back(b);
										}
										
										for(int k = 1; k < 4; k++) {
											if(bombers[j].loc_x - k < 0) break;
											c.x = bombers[j].loc_x - k;
											obstacle o = GetObstacleAtCoordinate(c);
											if(o.real) {
												count++;
												tempobs.push_back(o);
												break;
											}
											b = GetBombAtCoordinate(c);
											if(b.real) {
												count++;
												tempbombs.push_back(b);
											}
											bomberr = GetBomberAtCoordinate(c);
											if(bomberr.real) {
												count++;
												tempbombers.push_back(bomberr);
											}
										}
										
										for(int k = 1; k < 4; k++) {
											if(bombers[j].loc_x + k >= x) break;
											c.x = bombers[j].loc_x + k;
											obstacle o = GetObstacleAtCoordinate(c);
											if(o.real) {
												count++;
												tempobs.push_back(o);
												break;
											}
											b = GetBombAtCoordinate(c);
											if(b.real) {
												count++;
												tempbombs.push_back(b);
											}
											bomberr = GetBomberAtCoordinate(c);
											if(bomberr.real) {
												count++;
												tempbombers.push_back(bomberr);
											}
										}	
										
										c.x = bombers[j].loc_x;
										for(int k = 1; k < 4; k++) {
											if(bombers[j].loc_y - k  < 0) break;
											c.y = bombers[j].loc_y - k;
											obstacle o = GetObstacleAtCoordinate(c);
											if(o.real) {
												count++;
												tempobs.push_back(o);
												break;
											}
											b = GetBombAtCoordinate(c);
											if(b.real) {
												count++;
												tempbombs.push_back(b);
											}
											bomberr = GetBomberAtCoordinate(c);
											if(bomberr.real) {
												count++;
												tempbombers.push_back(bomberr);
											}
										}
										
										for(int k = 1; k < 4; k++) {
											if(bombers[j].loc_y + k  >= y) break;
											c.y = bombers[j].loc_y + k;
											obstacle o = GetObstacleAtCoordinate(c);
											if(o.real) {
												count++;
												tempobs.push_back(o);
												break;
											}
											b = GetBombAtCoordinate(c);
											if(b.real) {
												count++;
												tempbombs.push_back(b);
											}
											bomberr = GetBomberAtCoordinate(c);
											if(bomberr.real) {
												count++;
												tempbombers.push_back(bomberr);
											}
										}
																										
										o_message->data.object_count = count;
										int n = tempobs.size() + tempbombs.size() + tempbombers.size();
										od* o_data = new od[n];
										for(int k = 0; k < tempobs.size(); k++) {
											o_data[k].type = OBSTACLE;
											o_data[k].position.x = tempobs[k].loc_x;
											o_data[k].position.y = tempobs[k].loc_y;
										}
										for(int k = 0; k < tempbombs.size(); k++) {
											o_data[k + tempobs.size()].type = BOMB;
											o_data[k + tempobs.size()].position.x = tempbombs[k].loc_x;
											o_data[k + tempobs.size()].position.y = tempbombs[k].loc_y;									
										}
										for(int k = 0; k < tempbombers.size(); k++) {
											o_data[k + tempobs.size() + tempbombs.size()].type = BOMBER;
											o_data[k + tempobs.size() + tempbombs.size()].position.x = tempbombers[k].loc_x;
											o_data[k + tempobs.size() + tempbombs.size()].position.y = tempbombers[k].loc_y;									
										}								
										send_message(bombers[j].fd, o_message);
										send_object_data(bombers[j].fd, count, o_data);
										out->m = o_message;
										print_output(NULL, out, NULL, o_data);
										delete i_message;
										delete o_message;
										delete[] o_data;
										delete out;																									
										break;
									}
									
									case BOMBER_MOVE:
									{
										o_message->type = BOMBER_LOCATION;
										int targetX = i_message->data.target_position.x;
										int targetY = i_message->data.target_position.y;
										coordinate c;
										c.x = targetX;
										c.y = targetY;
										bool flag = getObstacle(c) || getBomber(c);
										if((absolute(targetX, bombers[j].loc_x) + absolute(targetY, bombers[j].loc_y)) == 1 && !flag && 0 <= targetX && targetX < x && 0 <= targetY && targetY < y) {
											o_message->data.new_position.x = targetX;
											o_message->data.new_position.y = targetY;
											bombers[j].loc_x = targetX;
											bombers[j].loc_y = targetY;
										}
										else {
											o_message->data.new_position.x = bombers[j].loc_x;
											o_message->data.new_position.y = bombers[j].loc_y;						
										}
										send_message(bombers[j].fd, o_message);
										out->m = o_message;
										print_output(NULL, out, NULL, NULL);
										delete i_message;
										delete o_message;
										delete out;
										break;
									}
									
									case BOMBER_PLANT:
									{
										coordinate c;
										c.x = bombers[j].loc_x;
										c.y = bombers[j].loc_y;
										o_message->type = BOMBER_PLANT_RESULT;
										if(!getBomb(c)) {
											int fd[2];
											PIPE(fd);
											bomb b;
											b.loc_x = bombers[j].loc_x;
											b.loc_y = bombers[j].loc_y;
											long inter = i_message->data.bomb_info.interval;
											b.r = i_message->data.bomb_info.radius;
											string arg = to_string(inter);
											string execname = "./bomb";
											char* arguments[3];
											arguments[0] = new char[execname.length() + 1];
											strcpy(arguments[0], execname.c_str());
											arguments[1] = new char[arg.length() + 1];
											strcpy(arguments[1], arg.c_str());
											arguments[2] = NULL;
											pid_t pid = fork();
											if(pid == 0) {
												dup2(fd[1], 0);
												dup2(fd[1], 1);
												close(fd[0]);
												close(fd[1]);
												execv(arguments[0], arguments);
											}
											b.pid = pid;
											b.fd = fd[0];
											b.real = true;
											bombs.push_back(b);
											close(fd[1]);
											o_message->data.planted = 1;
											for(int j = 0; j < 3; j++) {
												delete [] arguments[j];
											}	
										}
										else {
											o_message->data.planted = 0;
										}
										send_message(bombers[j].fd, o_message);
										out->m = o_message;
										print_output(NULL, out, NULL, NULL);
										delete i_message;
										delete o_message;
										delete out;
										break;
									}
								}
							}
							if(GetAliveBombersCount() > 1 && bombers[j].dead) {
								o_message->type = BOMBER_DIE;
								send_message(bombers[j].fd, o_message);
								out->m = o_message;
								print_output(NULL, out, NULL, NULL);
								close(bombers[j].fd);
								waitpid(bombers[j].pid, &stat, 0);
								bombers[j].reaped = true;
								delete i_message;
								delete o_message;
								delete out;
							}
							if(GetAliveBombersCount() <= 1) {
								if(bombers[j].winner) {
									o_message->type = BOMBER_WIN;
									send_message(bombers[j].fd, o_message);
									close(bombers[j].fd);
									waitpid(bombers[j].pid, &stat, 0);
									bombers[j].reaped = true;
								}
								else {
									o_message->type = BOMBER_DIE;
									send_message(bombers[j].fd, o_message);
									close(bombers[j].fd);
									waitpid(bombers[j].pid, &stat, 0);
									bombers[j].reaped = true;
								}
								out->m = o_message;
								print_output(NULL, out, NULL, NULL);
								delete i_message;
								delete o_message;
								delete out;


							}
						}
					}
				}
			}

			//delete pfds;
			sleep(0.001);
		}
	}
	else if(bomber_count == 1){
		im* i_message = new im;
		read_data(bombers[0].fd, i_message);
		imp *in = new imp;
		in->pid = bombers[0].pid;
		in->m = i_message;
		print_output(in, NULL, NULL, NULL);
		om* o_message = new om;
		o_message->type = BOMBER_WIN;
		send_message(bombers[0].fd, o_message);
		omp* out = new omp;
		out->pid = bombers[0].pid;
		out->m = o_message;
		print_output(NULL, out, NULL, NULL);
		delete i_message;
		delete in;
		delete out;
		close(bombers[0].fd);
		waitpid(bombers[0].pid, &stat, 0);
		return 0;
	}
	return 0;
}
