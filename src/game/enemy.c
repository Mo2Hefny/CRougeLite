/***************************************************************
 *
 *
 *
 *    ███████╗███╗   ██╗███████╗███╗   ███╗██╗   ██╗
 *    ██╔════╝████╗  ██║██╔════╝████╗ ████║╚██╗ ██╔╝
 *    █████╗  ██╔██╗ ██║█████╗  ██╔████╔██║ ╚████╔╝
 *    ██╔══╝  ██║╚██╗██║██╔══╝  ██║╚██╔╝██║  ╚██╔╝
 *    ███████╗██║ ╚████║███████╗██║ ╚═╝ ██║   ██║
 *    ╚══════╝╚═╝  ╚═══╝╚══════╝╚═╝     ╚═╝   ╚═╝
 *
 *     Enemy Module Header. (Game Object)
 *     Exposes the logic for the enemy object.
 *
 *     - Setup Enemy
 *     - Update Enemy
 *     - Draw Enemy
 *     - Clear Enemy
 *
 **************************************************************/

#include "enemy.h"
#include "../system/anime.h"
#include "../system/A-Star.h"
#include "../system/map.h"
#include "../system/draw.h"
#include "../system/midpoint.h"

#include <ctype.h>
#include <raylib.h>
#include <raymath.h>

// ***************************
// Private Function Prototypes
// ***************************
static Enemy *initEnemy(E_TYPE type, E_WEAPON weapon, Vector2 position);
static void deleteEnemy(int index);
static void animationController(Enemy *enemy);
static void updateStateMachine(Enemy *enemy, Vector2 *velocity, Vector2 *direction);

/**
 * setupEnemies - initialize all enemies manually to the game state
 */
void setupEnemies()
{
  const Settings *const settings = &(gameState->settings);
  initEnemy(E_SLIME, E_FIRE_BALL, (Vector2){200, 978});
  initEnemy(E_SLIME, E_FIRE_BALL, (Vector2){210, 840});
  initEnemy(E_SLIME, E_FIRE_BALL, (Vector2){1857.76, 410});
  initEnemy(E_SLIME, E_FIRE_BALL, (Vector2){1857.76, 675});
  initEnemy(E_FARMER, E_SWORD, (Vector2){975.22, 1268.8});
  initEnemy(E_CIVILIAN, E_SWORD, (Vector2){1315.141, 141});
  initEnemy(E_CIVILIAN, E_SWORD, (Vector2){1473, 135});
}

/**
 * drawEnemies - draw enemies on the scene with debug path visualization
 */
void drawEnemies()
{
  Enemy *enemies = gameState->enemies;
  int enemy_num = gameState->numOfEnemies;
  GameState *game_system = gameState;

  if (enemies == NULL)
    return;

  for (int i = 0; i < enemy_num; i++)
  {
    // Draw the enemy sprite
    bool flip = (enemies[i].drawDirection == -1) ? true : false;
    drawAnimator(&(enemies[i].object.animator), &(enemies[i].object.transform), WHITE, flip);

    // Draw health bar if the enemy was damaged recently
    Vector2 enemyCenter = {
        enemies[i].object.transform.position.x + enemies[i].object.collider.bounds.width / 2,
        enemies[i].object.transform.position.y};
    drawHealthBar(enemyCenter,
                  enemies[i].stats.health.currentHealth,
                  enemies[i].stats.health.maxHealth,
                  enemies[i].stats.health.lastUpdateTime);
  }
}

/**
 * updateEnemies - update the enemies objects every game tick
 */
void updateEnemies()
{
  Enemy *enemies = gameState->enemies;

  updateEnemyPath();

  Vector2 direction = {0, 0};

  for (int i = 0; i < gameState->numOfEnemies; i++)
  {
    if (enemies[i].stats.health.currentHealth <= 0 && enemies[i].object.animator.isFinished)
    {
      printf("Killed Enemy %s, which is %d of %d\n", enemies[i].name, i + 1, gameState->numOfEnemies);
      
      char *enemyName = strdup(enemies[i].name);
      for (char *c = enemyName; *c; c++) *c = tolower(*c);
      char *soundName = malloc(strlen(enemyName) + 6);
      sprintf(soundName, "%s_death", enemyName);

      playSoundEffect(soundName);
      free(enemyName);
      free(soundName);
      
      deleteEnemy(i);
      i--;
    }
  }

  for (int i = 0; i < gameState->numOfEnemies; i++)
  {
    Vector2 velocity = {0, 0};

    updateStateMachine(&enemies[i], &velocity, &direction);
    Vector2 position = Vector2Add(enemies[i].object.transform.position, velocity);

    if (Vector2Length(velocity) > 0)
    {
      enemies[i].isMoving = true;
    }
    else
    {
      enemies[i].isMoving = false;
    }

    if (velocity.x < 0)
    {
      enemies[i].drawDirection = -1;
    }
    else if (velocity.x > 0)
    {
      enemies[i].drawDirection = 1;
    }

    // NOTE: this makes the player unable to go out of frame
    enemies[i].object.rigidBody.velocity = velocity;
    // TODO: enemy clamping is removed for now restore later
    //  enemy->object.transform.position =
    //  Vector2Clamp(position, (Vector2){0, 0},
    //  (Vector2){GetScreenWidth() - 64, GetScreenHeight() - 64});
    enemies[i].object.transform.position = position;
    enemies[i].object.collider.bounds.x = position.x;
    enemies[i].object.collider.bounds.y = position.y;

    animationController(&enemies[i]);

    // FIXME: replace with sprite size
  }
}

/**
 * updateEnemyPath - update the path of all enemies to the closest
 **/
void updateEnemyPath()
{
  GameState *game_system = gameState;
  if (!game_system || game_system->numOfPlayers <= 0 || game_system->players[0].isMoving == false)
    return;

  Map *map = &(game_system->map);
  Player *player = &(game_system->players[0]);

  // Convert player position to grid coordinates
  Vector2 playerPos = player->object.transform.position;
  int colliderWidth = player->object.collider.bounds.width;
  int colliderHeight = player->object.collider.bounds.height;
  playerPos.x += colliderWidth / 2;
  playerPos.y += colliderHeight / 2;
  int playerRow = (int)((playerPos.y) / (map->tileHeight * map->scale));
  int playerCol = (int)((playerPos.x) / (map->tileWidth * map->scale));

  //--------------------------------------------------------
  int dx[] = {-1, 0, 1, 0, -1, -1, 1, 1};
  int dy[] = {0, -1, 0, 1, -1, 1, -1, 1};

  int playerCorners[4][2] = {
      {playerPos.x - colliderWidth / 2, playerPos.y - colliderHeight / 2}, // Top-left
      {playerPos.x + colliderWidth / 2, playerPos.y - colliderHeight / 2}, // Top-right
      {playerPos.x - colliderWidth / 2, playerPos.y + colliderHeight / 2}, // Bottom-left
      {playerPos.x + colliderWidth / 2, playerPos.y + colliderHeight / 2}  // Bottom-right
  };

  CoordPair surroundingCells[8];
  int numSurroundingCells = 0;

  for (int i = 0; i < 8; i++)
  {
    int newRow = playerRow + dy[i];
    int newCol = playerCol + dx[i];

    // Check if cell is valid and walkable
    if (isValid(newRow, newCol, map->numOfRows, map->numOfCols) && isWalkable(newRow, newCol))
    {
      surroundingCells[numSurroundingCells].first = newRow;
      surroundingCells[numSurroundingCells].second = newCol;
      numSurroundingCells++;
    }
  }

  // If no surrounding cells are available, use the player's cell
  if (numSurroundingCells == 0)
  {
    surroundingCells[0].first = playerRow;
    surroundingCells[0].second = playerCol;
    numSurroundingCells = 1;
  }
  //--------------------------------------------------------

  // For each enemy, calculate and draw path to closest surrounding cell
  for (int i = 0; i < game_system->numOfEnemies; i++)
  {

    Enemy *enemy = &(game_system->enemies[i]);

    // Skip dead enemies
    if (enemy->stats.health.currentHealth <= 0)
      continue;

    Vector2 enemyPos = enemy->object.transform.position;
    colliderWidth = enemy->object.collider.bounds.width;
    colliderHeight = enemy->object.collider.bounds.height;

    enemyPos.x += colliderWidth / 2;
    enemyPos.y += colliderHeight / 2;

    int enemyRow = (int)((enemyPos.y) / (map->tileHeight * map->scale));
    int enemyCol = (int)((enemyPos.x) / (map->tileWidth * map->scale));
    CoordPair enemyCoord = {.first = enemyRow, .second = enemyCol};

    //---------------------------Distance Check & line of sight---------------------------

    float distance = INT_MAX;

    for (int j = 0; j < 4; j++)
    {
      int cornerX = playerCorners[j][0];
      int cornerY = playerCorners[j][1];
      distance = fmin(distance, Vector2Distance(enemyPos, (Vector2){cornerX, cornerY}));
    }

    if (distance > enemy->ai.detectionRange || !lineOfSight((Vector2){enemyRow, enemyCol}, (Vector2){playerRow, playerCol}))
    {
      // Proceeds to last known path or stay idle
      enemy->ai.inLineOfSight = NULL;
      continue;
    }
    //---------------------------------------------------------------------------------
    enemy->ai.inLineOfSight = player;

    // Find path to each surrounding cell and keep the shortest one
    CoordPair *shortestPath = NULL;
    int shortestPathLength = 0;

    for (int j = 0; j < numSurroundingCells; j++)
    {
      int pathLength = 0;
      CoordPair *path = aStarSearch(enemyCoord, surroundingCells[j], &pathLength);

      if (path && (shortestPath == NULL || pathLength < shortestPathLength))
      {
        if (shortestPath)
        {
          free(shortestPath);
        }
        shortestPath = path;
        shortestPathLength = pathLength;
      }
      else if (path)
      {
        free(path);
      }
    }

    if (enemy->ai.path)
    {
      free(enemy->ai.path);
    }
    enemy->ai.path = shortestPath;
    enemy->ai.pathLength = shortestPathLength;
    enemy->ai.currentPathIndex = 1;
  }
}

/**
 * clearEnemies - free enemies array from heap
 */
void clearEnemies()
{
  printf("Deleting Enemies\n");
  while (gameState->numOfEnemies > 0)
  {
    deleteEnemy(0);
  }
  free(gameState->enemies);
  gameState->enemies = NULL;
  printf("Deleted all Enemies\n");
}

// *****************
// PRIVATE FUNCTIONS
// *****************

/**
 * initEnemy - initialize a new enemy object and add it to the game sate
 *
 * @param type The enemy type
 * @param weapon The enemy weapon
 * @param position The enemy spawn position
 *
 * @return Pointer to the new enemy object
 */
static Enemy *initEnemy(E_TYPE type, E_WEAPON weapon, Vector2 position)
{
  static unsigned int ID = 0;
  Dictionary *dict = gameState->enemyDictionary;
  Enemy *enemy = &(gameState->enemies[gameState->numOfEnemies++]);
  int l = 0, r = NUM_OF_E_TYPE - 1;

  while (l <= r)
  {
    int mid = l + (r - l) / 2;
    int cmp = dict[mid].opcode - type;
    if (!cmp)
    {
      *enemy = dict[mid].entry.enemy;
      printf("Added enemy of type: %s\n", enemy->name);
      break;
    }
    if (cmp < 0)
      l = mid + 1;
    else
      r = mid - 1;
  }

  enemy->object.animator = (Animator){
      .isFinished = false,
      .currentState = IDLE,
  };

  enemy->ai.patrolStart = (Vector2){0, 0};
  enemy->ai.patrolEnd = (Vector2){0, 0};
  enemy->ai.lastAttackTime = 0.0f;
  enemy->ai.path = NULL;
  enemy->ai.currentPathIndex = 0;
  enemy->ai.pathLength = 0;

  switch (type)
  {
  case E_CIVILIAN:
    enemy->object.animator.animations[IDLE] = (SpriteAnimation){
        .frameNames =
            {
                "vampire_1",
                "vampire_2",
                "vampire_3",
                "vampire_4",
            },
        .numOfFrames = 4,
        .fps = 8,
        .isLooping = true,
        .isFinished = false,
        .currentFrame = 0,
        .frameCount = 0,
    };
    break;
  case E_FARMER:
    enemy->object.animator.animations[IDLE] = (SpriteAnimation){
        .frameNames =
            {
                "skeleton_1",
                "skeleton_2",
                "skeleton_3",
                "skeleton_4",
            },
        .numOfFrames = 4,
        .fps = 8,
        .isLooping = true,
        .isFinished = false,
        .currentFrame = 0,
        .frameCount = 0,
    };
    break;
  case E_SLIME:
  default:
    enemy->object.animator.animations[IDLE] = (SpriteAnimation){
        .frameNames =
            {
                "slime_1_0",
                "slime_1_1",
                "slime_1_2",
                "slime_1_3",
            },
        .numOfFrames = 4,
        .fps = 8,
        .isLooping = true,
        .isFinished = false,
        .currentFrame = 0,
        .frameCount = 0,
    };
    enemy->object.animator.animations[TAKE_DAMAGE] = (SpriteAnimation){
        .frameNames =
            {
                "slime_2_0",
                "slime_2_1",
                "slime_2_2",
                "slime_2_3",
            },
        .numOfFrames = 4,
        .fps = 8,
        .isLooping = false,
        .isFinished = false,
        .currentFrame = 0,
        .frameCount = 0,
    };
    enemy->object.animator.animations[DIE] = (SpriteAnimation){
        .frameNames =
            {
                "slime_0_2",
                "slime_0_1",
                "slime_0_0",
            },
        .numOfFrames = 3,
        .fps = 8,
        .isLooping = false,
        .isFinished = false,
        .currentFrame = 0,
        .frameCount = 0,
    };
    break;
  }

  enemy->ID = ID++;
  enemy->type = type;
  enemy->object.transform.position = position;
  enemy->object.collider.bounds.x = position.x;
  enemy->object.collider.bounds.y = position.y;
  enemy->object.transform.scale = (Vector2){4, 4};
  enemy->object.rigidBody.type = BODY_GHOST;
  
  enemy->attackCount = 0;
  enemy->lastAttackTime = 0.0f;
  
  enemy->weapon = initWeapon(weapon, false);

  return enemy;
}

/**
 * updateStateMachine - update the enemy state machine
 *
 * @param enemy Pointer to the enemy object
 * @param velocity Pointer to the velocity vector
 * @param direction Pointer to the direction vector
 */
static void updateStateMachine(Enemy *enemy, Vector2 *velocity, Vector2 *direction)
{
  EnemyAI *ai = &(enemy->ai);
  switch (ai->state)
  {
  case RUN:
    if (ai->path && enemy->stats.health.currentHealth > 0)
    {
      CoordPair nextStep = ai->path[ai->currentPathIndex];
      Vector2 enemyPos = enemy->object.transform.position;
      int colliderWidth = enemy->object.collider.bounds.width;
      int colliderHeight = enemy->object.collider.bounds.height;
      int cellWidth = gameState->map.tileWidth * gameState->map.scale;
      int cellHeight = gameState->map.tileHeight * gameState->map.scale;

      Rectangle cellBounds = {
          nextStep.second * cellWidth, // x position
          nextStep.first * cellHeight, // y position
          cellWidth,                   // width
          cellHeight                   // height
      };

      Vector2 enemyCenterPos = {
          enemyPos.x + colliderWidth / 2,
          enemyPos.y + colliderHeight / 2};

      Vector2 nextCellPos = {
          nextStep.second * cellWidth + cellWidth / 2,
          nextStep.first * cellHeight + cellHeight / 2};
      *direction = Vector2Subtract(nextCellPos, enemyCenterPos);

      float distance = Vector2Length(*direction);

      bool insideCell =
          enemyCenterPos.x >= cellBounds.x &&
          enemyCenterPos.x <= cellBounds.x + cellBounds.width &&
          enemyCenterPos.y >= cellBounds.y &&
          enemyCenterPos.y <= cellBounds.y + cellBounds.height;

      if (insideCell)
      {
        ai->currentPathIndex++;
        // If we still have waypoints, calculate new direction
        int randomPaddedDistance = GetRandomValue(0, 3 * ai->minDistanceToAttack != 0) + ai->minDistanceToAttack;
        if (ai->currentPathIndex < ai->pathLength - (ai->inLineOfSight != NULL) * randomPaddedDistance)
        {
          nextStep = ai->path[ai->currentPathIndex];
          nextCellPos = (Vector2){
              nextStep.second * cellWidth + cellWidth / 2,
              nextStep.first * cellHeight + cellHeight / 2};
          *direction = Vector2Subtract(nextCellPos, enemyCenterPos);
        }
        else
        {
          // Path complete
          free(ai->path);
          ai->path = NULL;
          ai->currentPathIndex = 0;
          ai->pathLength = 0;
          *direction = (Vector2){0, 0};
          ai->state = ai->inLineOfSight != NULL ? ATTACK : IDLE;
        }
      }

      // Normalize direction and scale by speed
      if (Vector2Length(*direction) > 0) *velocity = Vector2Scale(Vector2Normalize(*direction), enemy->stats.speed);
    } else {
      ai->state = IDLE; // No path available, go idle
    }
    break;
  case ATTACK:
    if (ai->inLineOfSight)
    {
      // Check if target is still within reasonable attack range
      Vector2 enemyPos = {
          enemy->object.collider.bounds.x + enemy->object.collider.bounds.width / 2,
          enemy->object.collider.bounds.y + enemy->object.collider.bounds.height / 2};
      Vector2 targetPos = {
          ai->inLineOfSight->object.collider.bounds.x + ai->inLineOfSight->object.collider.bounds.width / 2,
          ai->inLineOfSight->object.collider.bounds.y + ai->inLineOfSight->object.collider.bounds.height / 2};
      
      float distanceToTarget = Vector2Distance(enemyPos, targetPos);
      int randomPaddedDistance = GetRandomValue(0, 3) + ai->minDistanceToAttack;
      float maxAttackRange = (randomPaddedDistance + 2) * (gameState->map.tileWidth * gameState->map.scale);
      
      // If target is too far, switch back to RUN state to chase
      if (distanceToTarget > maxAttackRange || (ai->minDistanceToAttack == 0 && distanceToTarget > 100.0f)) {
        ai->state = RUN;
        break;
      }
      
      if (ai->lastAttackTime + ai->attackCooldown >= GetTime()) break;

      ai->lastAttackTime = GetTime();
      setState(&(enemy->object.animator), ATTACK);

      Weapon *weapon = &(enemy->weapon);
      float deltaTime = GetFrameTime();
      Vector2 srcPos = enemyPos;
      Vector2 destPos = targetPos;
      if (weapon->type == RANGED_WEAPON) {
        if (weapon->weapon.ranged.bulletInfo.isTracking) {
          weapon->weapon.ranged.bulletInfo.targetID = ai->inLineOfSight->ID;
        } else {
          weapon->weapon.ranged.bulletInfo.targetID = -1;
        }
        updateRangedWeapon(weapon, true, enemy->ID, srcPos, destPos,
                       deltaTime, false);
      } else if (weapon->type == MELEE_WEAPON) {
        updateMeleeWeapon(weapon, true, enemy->ID, srcPos, destPos,
                        deltaTime, false);
      }
    } else {
      ai->state = IDLE;
    }
    break;
  case IDLE:
  default:
    if (ai->inLineOfSight && ai->inLineOfSight->stats.health.currentHealth > 0) ai->state = RUN;
  }
}

/**
 * animationController - controls the animation state of the enemy
 */
static void animationController(Enemy *enemy)
{
  Animator *animator = &(enemy->object.animator);

  if (animator->isFinished == true)
    setState(animator, IDLE);

  updateAnimator(animator);
}

/**
 * deleteEnemy - remove the enemy object from the game state
 *
 * @param index Index of the enemy in the array
 */
static void deleteEnemy(int index)
{
  Enemy *enemy = &gameState->enemies[index];

  // Free the AI path if it exists
  if (enemy->ai.path)
  {
    free(enemy->ai.path);
    enemy->ai.path = NULL;
  }

  // Note: enemy->name is NOT freed here because it points to string literals
  // from the enemy dictionary (e.g., "Civilian", "Farmer", "Slime")
  // These are stored in read-only memory and should never be freed

  gameState->enemies[index] = gameState->enemies[--(gameState->numOfEnemies)];
}
