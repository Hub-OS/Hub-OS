namespace Overworld {
  class Actor; // namespace Overworld::Actor;
  class Map;

  class PlayerController {
    Overworld::Actor* actor{ nullptr };
    bool listen{ true };
  public:
    void ControlActor(Actor& actor);
    void ReleaseActor();
    void Update(double elapsed);
    void ListenToInputEvents(const bool listen);
  };
}