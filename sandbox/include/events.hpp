#ifndef SANDBOX_EVENTS_HPP
#define SANDBOX_EVENTS_HPP

namespace Sandbox
{
  // Game events
  struct ScoreChangedEvent        { int score; };
  struct PlayerDiedEvent          {};
  struct WaveCompletedEvent       { int wave; };
  struct AsteroidDestroyedEvent   { int generation; float x; float y; };
  struct RestartEvent             {};

  // Debug events
  struct SpawnAsteroidEvent       {};
  struct ToggleInvincibilityEvent {};
} // namespace Sandbox

#endif // SANDBOX_EVENTS_HPP
