const fs = require('fs');
const https = require('https');

const query = `
query {
  pokemon_v2_pokemon(limit: 151) {
    id
    name
    pokemon_v2_pokemontypes {
      pokemon_v2_type {
        name
      }
    }
    pokemon_v2_pokemonstats {
      base_stat
      pokemon_v2_stat {
        name
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

const req = https.request(options, res => {
  let d = '';
  res.on('data', chunk => d += chunk);
  res.on('end', () => {
    try {
      const response = JSON.parse(d);
      let sql = "INSERT OR IGNORE INTO pokemon VALUES \n";
      const pokemons = response.data.pokemon_v2_pokemon;
      
      const values = pokemons.map(p => {
        const id = p.id;
        const name = p.name.charAt(0).toUpperCase() + p.name.slice(1);
        const type1 = p.pokemon_v2_pokemontypes[0]?.pokemon_v2_type.name || 'Normal';
        const type2 = p.pokemon_v2_pokemontypes[1]?.pokemon_v2_type.name || null;
        
        // stats array order in PokeAPI: hp, attack, defense, sp-attack, sp-defense, speed
        const hp = p.pokemon_v2_pokemonstats[0]?.base_stat || 0;
        const attack = p.pokemon_v2_pokemonstats[1]?.base_stat || 0;
        const defense = p.pokemon_v2_pokemonstats[2]?.base_stat || 0;
        const sp_attack = p.pokemon_v2_pokemonstats[3]?.base_stat || 0;
        const sp_defense = p.pokemon_v2_pokemonstats[4]?.base_stat || 0;
        const speed = p.pokemon_v2_pokemonstats[5]?.base_stat || 0;
        
        const type1Str = type1.charAt(0).toUpperCase() + type1.slice(1);
        const type2Str = type2 ? `'${type2.charAt(0).toUpperCase() + type2.slice(1)}'` : 'NULL';
        
        return `(${id}, '${name}', '${type1Str}', ${type2Str}, 'Kanto', 'KANTO', '...', 'romfs:/sprites/${id.toString().padStart(3, '0')}.png', ${hp}, ${attack}, ${defense}, ${sp_attack}, ${sp_defense}, ${speed})`;
      });
      
      sql += values.join(",\n") + ";";
      fs.writeFileSync('kanto_inserts.sql', sql);
      console.log('Done! Generated kanto_inserts.sql');
    } catch (e) {
      console.error(e);
    }
  });
});

req.on('error', e => console.error(e));
req.write(data);
req.end();
