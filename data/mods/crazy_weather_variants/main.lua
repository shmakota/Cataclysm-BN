local mod = game.mod_runtime[game.current_mod]

mod.mutation_chance = 0.08
mod.mutation_message_chance = 0.25

mod.on_weather_updated = function(params)
  if params.weather_id ~= "mutagenic_rain" then return end
  if params.is_sheltered then return end

  local player = gapi.get_avatar()
  if not player or not player:is_avatar() then return end

  local roll = math.random()
  if roll <= mod.mutation_chance then
    player:mutate()
    gapi.add_msg(MsgType.bad, locale.gettext("The mutagenic rain twists your body into something unfamiliar."))
  elseif roll <= mod.mutation_chance + mod.mutation_message_chance then
    gapi.add_msg(MsgType.warning, locale.gettext("Mutagenic rain stings your exposed flesh."))
  end
end
