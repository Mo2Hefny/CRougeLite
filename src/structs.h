/****************************************************************
 *
 *
 *    ███████╗████████╗██████╗ ██╗   ██╗ ██████╗████████╗███████╗
 *    ██╔════╝╚══██╔══╝██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔════╝
 *    ███████╗   ██║   ██████╔╝██║   ██║██║        ██║   ███████╗
 *    ╚════██║   ██║   ██╔══██╗██║   ██║██║        ██║   ╚════██║
 *    ███████║   ██║   ██║  ██║╚██████╔╝╚██████╗   ██║   ███████║
 *    ╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝  ╚═════╝   ╚═╝   ╚══════╝
 *
 *
 *   Structs and Enums for the game.
 *
 *
 *****************************************************************/

#ifndef STRUCTS_H
#define STRUCTS_H

#include "defs.h"
#include <raylib.h>

// TODO: Make enum for all stats related to the specified types
// instead of no encapsulation.

typedef struct {
  Vector2 position;
  float rotation;
  Vector2 scale;
} Transform;

typedef struct {
  Vector2 velocity;
  Vector2 acceleration;
  float drag;
  bool isKinematic;       // Kinematic object is typically not affected by physics 
                          // forces but can still interact with other objects in certain ways.
} RigidBody2D;

typedef struct {
  Vector2 offset;
  float width;
  float height;
} Collider2D;

typedef struct {
  Texture2D texture;
  int width;
  int height;
} SpriteRenderer;

typedef struct {
  Texture2D* animations;     // Idk what is the type of the animation sprites.
  int currentFrame;
  int totalFrame;
  float frameTime;
  float elapsedTime;      // Time elapsed since the last frame change.
  bool loop;          // NOTE: still not used
  bool finished;      // NOTE: still not used
} Animator;

typedef struct {
  int currentHealth;
  int maxHealth;
} Health;

typedef struct {
  int power;
  float speed;
  float cooldown;
} Attack;

typedef struct {
  int value;
  int nearHitValue;       // Blocked on the last second.
  // TODO: Add defense for different type of attacks?
} Defense;

typedef struct {
  int xp;
  int level;
} Experience;

typedef struct {
  Health health;
  Attack attack;
  Defense defense;
} Stats;

typedef struct {
  int up;
  int down;
  int left;
  int right;
  int shoot;
  int action;
} Input;

typedef struct {
  int damage;
  float fireRate;
  float lastShotTime;
  SpriteRenderer weaponSprite;
} Weapon;

typedef struct {
  int MAX_NUM_OF_WEAPONS;
  int currentNumOfWeapons;
  Weapon* weapons;
} Inventory;

typedef struct {
  Transform transform;
  RigidBody2D rigidBody;
  Collider2D collider;
  SpriteRenderer spriteRenderer;
  Animator animator;
  Stats stats;
  Weapon weapon;
} GameObject;

typedef enum {
  PATROL,
  IDLE,
  CHASE,
  ATTACK,
  FLEE
} State;

typedef struct {
  Vector2 patrolStart;
  Vector2 patrolEnd;
  float detectionRange;
  float attackCooldown;
  float lastAttackTime;
  float dodgePercentage;        // Dodge or Parry or Block. Or do these three separately??
  float speed;
  State state;
} EnemyAI;

typedef struct {
  GameObject object;
  EnemyAI ai;
} Enemy;

typedef enum
{
  WEREWOLF,
  PYROMANIAC,
  KNIGHT,
  NUM_OF_P_TYPE
} P_TYPE;

typedef enum
{
  LONG_SWORD,
  NUM_OF_P_WEAPON
} P_WEAPON;

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT,
} DIRECTIONS;

typedef struct
{
  int playerID;
  float bulletSpeed;
  float bulletDamage;
  float bulletRange;
  float bulletHealth;
  Vector2 startPosition;      // To know if the bullet exceeded the range.
  Transform transform;
  SpriteRenderer bulletSprite;
} Bullet;

typedef struct {
  int playerID;
  float slashRange;
  float slashDamage;
  int isActive;
  Transform transform;
  SpriteRenderer slashSprite;
} Slash;


typedef union {
  Bullet bullet;
  Slash slash;
} CombatActionUnion;

typedef enum {
  ACTION_NONE,
  ACTION_BULLET,
  ACTION_SLASH
} CombatActionType;

typedef struct {
  CombatActionUnion action;
  CombatActionType type;
} CombatAction;

typedef struct
{
  // Player Info
  char *name;
  int ID;
  // Player Selection
  P_TYPE type;
  P_WEAPON weapon;
  Texture2D texture;

  // Player Stats
  GameObject object;
  Input input;
  int score;
  int fire;
  int drawDirection;    // 1 for right, -1 for left
  bool isMoving;
  DIRECTIONS direction; // to get info on the direction the player is facing.
} Player;

typedef enum
{
  E_CIVILIAN,
  E_FARMER,
  E_KNIGHT,
  NUM_OF_E_TYPE
} E_TYPE;

typedef enum
{
  NUM_OF_E_WEAPON
} E_WEAPON;

typedef struct
{
  int screenWidth;
  int screenHeight;
  bool fullscreen;
  int musicVolume;
  int soundVolume;
  bool music_on;
  bool sfx_on;
} Settings;

typedef struct AtlasImage
{
  char *filename;
  Rectangle source;
  Vector2 origin;
  struct AtlasImage *next;
} AtlasImage;

typedef struct
{
  int numOfPlayers;
  Player *players;
  
  int numOfEnemies;
  Enemy *enemies;

  int numOfCombatActions;
  CombatAction *combatActions;

  int level;
  bool isGameOver;
  bool isFinished;
  Texture2D atlasTexture;
  AtlasImage *atlasImages;

  Settings settings;
} GameState;
#endif // STRUCTS_H
