const fs = require('fs');
const https = require('https');

const query = `
query {
  pokemon_v2_pokemon(limit: 1025) {
    id
    name
    pokemon_v2_pokemontypes {
      pokemon_v2_type {
        name
      }
    }
    pokemon_v2_pokemonstats {
      base_stat
    }
    pokemon_v2_pokemonabilities {
      pokemon_v2_ability {
        name
      }
    }
    pokemon_v2_pokemonspecy {
      pokemon_v2_growthrate {
        name
      }
      pokemon_v2_pokemonevolutions(limit: 1) {
        min_level
      }
      pokemon_v2_pokemonspeciesflavortexts(where: {language_id: {_eq: 9}}, limit: 1) {
        flavor_text
      }
    }
  }
}
`;

const data = JSON.stringify({ query });

const options = {
  hostname: 'beta.pokeapi.co',
  path: '/graphql/v1beta',
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Content-Length': Buffer.byteLength(data)
  }
};

const getRegionAndGame = (id) => {
  if (id <= 151) return ['Kanto', 'KANTO'];
  if (id <= 251) return ['Johto', 'JOHTO'];
  if (id <= 386) return ['Hoenn', 'HOENN'];
  if (id <= 493) return ['Sinnoh', 'BDSP'];
  if (id <= 649) return ['Unova', 'UNOVA'];
  if (id <= 721) return ['Kalos', 'KALOS'];
  if (id <= 809) return ['Alola', 'ALOLA'];
  if (id <= 898) return ['Galar', 'SWSH'];
  if (id <= 905) return ['Hisui', 'PLA'];
  return ['Paldea', 'SV'];
};

const req = https.request(options, res => {
  let d = '';
  res.on('data', chunk => d += chunk);
  res.on('end', () => {
    try {
      const response = JSON.parse(d);
      const pokemons = response.data.pokemon_v2_pokemon;
      let sql = "BEGIN TRANSACTION;\n";
      
      const values = pokemons.map(p => {
        const id = p.id;
        const name = p.name.charAt(0).toUpperCase() + p.name.slice(1);
        const type1 = p.pokemon_v2_pokemontypes[0]?.pokemon_v2_type.name || 'Normal';
        const type2 = p.pokemon_v2_pokemontypes[1]?.pokemon_v2_type.name || null;
        
        const hp = p.pokemon_v2_pokemonstats[0]?.base_stat || 0;
        const attack = p.pokemon_v2_pokemonstats[1]?.base_stat || 0;
        const defense = p.pokemon_v2_pokemonstats[2]?.base_stat || 0;
        const sp_attack = p.pokemon_v2_pokemonstats[3]?.base_stat || 0;
        const sp_defense = p.pokemon_v2_pokemonstats[4]?.base_stat || 0;
        const speed = p.pokemon_v2_pokemonstats[5]?.base_stat || 0;
        
        const type1Str = type1.charAt(0).toUpperCase() + type1.slice(1);
        const type2Str = type2 ? `'${type2.charAt(0).toUpperCase() + type2.slice(1)}'` : 'NULL';
        
        const [region, gameCode] = getRegionAndGame(id);
        
        let desc = 'No description available.';
        if (p.pokemon_v2_pokemonspecy?.pokemon_v2_pokemonspeciesflavortexts?.[0]) {
          desc = p.pokemon_v2_pokemonspecy.pokemon_v2_pokemonspeciesflavortexts[0].flavor_text
                  .replace(/[\n\f\r]/g, ' ')
                  .replace(/'/g, "''");
        }
        
        const abilities = p.pokemon_v2_pokemonabilities || [];
        const ability1 = abilities[0] ? `'${abilities[0].pokemon_v2_ability.name.replace(/'/g, "''")}'` : 'NULL';
        const ability2 = abilities.length > 1 && !abilities[1].is_hidden ? `'${abilities[1].pokemon_v2_ability.name.replace(/'/g, "''")}'` : 'NULL';
        const hiddenAbility = abilities.length > 1 && abilities[abilities.length - 1].is_hidden ? `'${abilities[abilities.length - 1].pokemon_v2_ability.name.replace(/'/g, "''")}'` : 'NULL';
        
        const growthRate = p.pokemon_v2_pokemonspecy?.pokemon_v2_growthrate?.name || 'medium-fast';
        const evos = p.pokemon_v2_pokemonspecy?.pokemon_v2_pokemonevolutions || [];
        const evolveLevel = evos.length > 0 && evos[0].min_level ? evos[0].min_level : 'NULL';

        return `INSERT OR REPLACE INTO pokemon (species_id, name, type1, type2, region, game_code, description, sprite_path, hp, attack, defense, sp_attack, sp_defense, speed, growth_rate, ability1, ability2, hidden_ability, evolve_level) VALUES (${id}, '${name}', '${type1Str}', ${type2Str}, '${region}', '${gameCode}', '${desc}', 'romfs:/sprites/${id.toString().padStart(3, '0')}.png', ${hp}, ${attack}, ${defense}, ${sp_attack}, ${sp_defense}, ${speed}, '${growthRate}', ${ability1}, ${ability2}, ${hiddenAbility}, ${evolveLevel});`;
      });
      
      sql += values.join("\n") + "\nCOMMIT;";
      
      if (!fs.existsSync('romfs/db')) {
        fs.mkdirSync('romfs/db', { recursive: true });
      }
      fs.writeFileSync('romfs/db/pokemon_seed.sql', sql);
      console.log('Done! Generated romfs/db/pokemon_seed.sql');
    } catch (e) {
      console.error(e);
    }
  });
});

req.on('error', e => console.error(e));
req.write(data);
req.end();
