import random  
from typing import Optional  
import logging
from PIL import Image, ImageTk
from PIL.Image import Resampling 
from wingedsheep.carcassonne.carcassonne_game import CarcassonneGame  
from wingedsheep.carcassonne.carcassonne_game_state import CarcassonneGameState  
from wingedsheep.carcassonne.objects.actions.action import Action  
from wingedsheep.carcassonne.tile_sets.supplementary_rules import SupplementaryRule  
from wingedsheep.carcassonne.tile_sets.tile_sets import TileSet  

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('carcassonne_game.log'),
        logging.StreamHandler()  # This will also print to console
    ]
)

logger = logging.getLogger(__name__)

game = CarcassonneGame(  
    players=2,  
    tile_sets=[TileSet.BASE, TileSet.THE_RIVER, TileSet.INNS_AND_CATHEDRALS],  
    supplementary_rules=[SupplementaryRule.ABBOTS, SupplementaryRule.FARMERS]  
)  

logger.info("Game started")
  
while not game.is_finished():  
    player: int = game.get_current_player()  
    valid_actions: list[Action] = game.get_possible_actions() 
    logger.info(f"Player {player} - Available actions: {len(valid_actions)}")

    logger.info(f"Player {player} - num_actions: {len(valid_actions)}")
    action: Optional[Action] = random.choice(valid_actions)  
    if action is not None:  
        logger.info(f"Player {player} took action: {action}")
        game.step(player, action) 
    else: 
        break 
    game.render()

logger.info("Game finished")